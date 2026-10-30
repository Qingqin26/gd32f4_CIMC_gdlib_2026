/************************************************************
 * 文件名: ad3344_app.c
 * 功能:   AD3344 + PT100 应用层实现
 * 平台:   2026 CIMC GD32F4xx
 ************************************************************/
#include "ad3344_app.h"
#include "gd30ad3344.h"
#include "systick.h"

/* AD3344 用户配置 (差分 AIN0-AIN1 + PGA 4.096V + 100SPS + 连续转换) */
#define AD3344_USER_CONFIG  (AD3344_REG_CONFIG_OS_SINGLE        |  \
                             AD3344_REG_CONFIG_MUX_DIFF_0_1     |  \
                             AD3344_REG_CONFIG_PGA_4_096V       |  \
                             AD3344_REG_CONFIG_MODE_CONTIN      |  \
                             AD3344_REG_CONFIG_DR_100SPS        |  \
                             AD3344_REG_CONFIG_PULL_UP_EN       |  \
                             AD3344_REG_CONFIG_NOP_VALID        |  \
                             AD3344_RESERVED_VALUE)

/************************************************************
 * 静态函数: SPI3 + GPIO 底层初始化
 ************************************************************/
static void ad3344_gpio_spi_init(void)
{
    spi_parameter_struct spi_init_struct;

    /* 使能时钟 */
    rcu_periph_clock_enable(AD3344_GPIO_RCU);
    rcu_periph_clock_enable(AD3344_SPI_RCU);

    /* PE12/13/14 复用为 SPI3 (AF5) */
    gpio_af_set(AD3344_GPIO_PORT, GPIO_AF_5,
                AD3344_SCK_PIN | AD3344_MISO_PIN | AD3344_MOSI_PIN);
    gpio_mode_set(AD3344_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE,
                  AD3344_SCK_PIN | AD3344_MISO_PIN | AD3344_MOSI_PIN);
    gpio_output_options_set(AD3344_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ,
                            AD3344_SCK_PIN | AD3344_MISO_PIN | AD3344_MOSI_PIN);

    /* PE9 = CS, 推挽输出, 默认拉高 (释放) */
    gpio_mode_set(AD3344_GPIO_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, AD3344_CS_PIN);
    gpio_output_options_set(AD3344_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, AD3344_CS_PIN);
    SPI_SET_CS();

    /* SPI3 配置: 主机, 16-bit, 模式1, 软件NSS, MSB first, 分频16 */
    spi_i2s_deinit(AD3344_SPI);
    spi_init_struct.trans_mode           = SPI_TRANSMODE_FULLDUPLEX;
    spi_init_struct.device_mode          = SPI_MASTER;
    spi_init_struct.frame_size           = SPI_FRAMESIZE_16BIT;
    spi_init_struct.clock_polarity_phase = SPI_CK_PL_LOW_PH_2EDGE;
    spi_init_struct.nss                  = SPI_NSS_SOFT;
    spi_init_struct.prescale             = SPI_PSC_16;
    spi_init_struct.endian               = SPI_ENDIAN_MSB;
    spi_init(AD3344_SPI, &spi_init_struct);

    spi_enable(AD3344_SPI);
}

/************************************************************
 * 函数: ad3344_spi_txrx16bit
 * 功能: SPI3 16位收发 (阻塞)
 * 注意: 此函数被 gd30ad3344.c 调用
 ************************************************************/
uint16_t ad3344_spi_txrx16bit(uint16_t data)
{
    while (spi_i2s_flag_get(AD3344_SPI, SPI_FLAG_TBE) == RESET) { }
    spi_i2s_data_transmit(AD3344_SPI, data);

    while (spi_i2s_flag_get(AD3344_SPI, SPI_FLAG_RBNE) == RESET) { }
    return (uint16_t)spi_i2s_data_receive(AD3344_SPI);
}

/************************************************************
 * 函数: ad3344_app_init
 * 功能: 应用层初始化
 ************************************************************/
void ad3344_app_init(void)
{
    ad3344_gpio_spi_init();

    /* 等 AD3344 上电稳定 */
    delay_1ms(10);

    /* 保存全局配置 (gd30ad3344 模块使用) */
    AD3344_CONFIG = AD3344_USER_CONFIG;

    /* 写入 CONFIG 寄存器, 启动转换 */
    ad3344_init(AD3344_USER_CONFIG);

    /* 等首次转换完成 (100SPS 一次约 10ms, 留 50ms 余量) */
    delay_1ms(50);
}

/************************************************************
 * 函数: ad3344_app_read_raw
 * 功能: 读取一次 ADC raw 值 (16-bit 有符号)
 ************************************************************/
int16_t ad3344_app_read_raw(void)
{
    uint16_t raw;

    SPI_CLR_CS();
    delay_1us(10);
    raw = ad3344_spi_txrx16bit(0x0000);
    SPI_SET_CS();

    return (int16_t)raw;
}

/************************************************************
 * 静态函数: 软件平方根 (避免依赖 math.h)
 * 算法: 牛顿迭代
 ************************************************************/
static float ad3344_sqrt(float x)
{
    float sr;
    float prev;
    int   i;

    if (x <= 0.0f) {
        return 0.0f;
    }

    sr = (x > 1.0f) ? x : 1.0f;
    for (i = 0; i < 20; i++) {
        prev = sr;
        sr   = 0.5f * (sr + x / sr);
        if (prev > sr ? (prev - sr) < 1e-7f : (sr - prev) < 1e-7f) {
            break;
        }
    }
    return sr;
}

/************************************************************
 * 函数: ad3344_app_read_temp
 * 功能: 读取 PT100 温度 (摄氏度)
 * 计算: raw -> V -> R -> T (Callendar-Van Dusen)
 ************************************************************/
float ad3344_app_read_temp(void)
{
    float t;
    ad3344_app_read_all(0, 0, &t);
    return t;
}

/************************************************************
 * 函数: ad3344_app_read_all
 * 功能: 一次读取，返回电压/电阻/温度 (单次 SPI 读取，比 3 次省)
 * 参数: voltage_V      - 输出电压 V (NULL 跳过)
 *       resistance_ohm - PT100 电阻 (NULL 跳过)
 *       temp_C         - 温度 摄氏度 (NULL 跳过)
 ************************************************************/
void ad3344_app_read_all(float *voltage_V, float *resistance_ohm, float *temp_C)
{
    const float R0 = 100.0f;
    const float A  = 3.9083e-3f;
    const float B  = -5.775e-7f;

    int16_t raw;
    float   V;
    float   R;
    float   T;
    float   discriminant;

    raw = ad3344_app_read_raw();
    V   = (float)raw * (4.096f / 32768.0f * 0.994); // raw 按 ±4.096V 量程线性换算为电压
    R   = V * 1000.0f;

    discriminant = A * A - 4.0f * B * (1.0f - R / R0);
    if (discriminant < 0.0f) {
        T = -999.0f;
    } else {
        T = (-A + ad3344_sqrt(discriminant)) / (2.0f * B);
    }

    if (voltage_V      != 0) { *voltage_V      = V; }
    if (resistance_ohm != 0) { *resistance_ohm = R; }
    if (temp_C         != 0) { *temp_C         = T; }
}
/****************************End*****************************/
