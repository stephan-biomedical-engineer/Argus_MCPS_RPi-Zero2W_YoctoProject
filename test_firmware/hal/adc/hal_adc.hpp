#ifndef HAL_ADC_HPP
#define HAL_ADC_HPP

#include <cstdint>
#include "hal_i2c.hpp"
#include "hal_gpio.hpp"

/* I2C Slave Addresses */
#define ADS1115_ADDR_GND    0x48 // 1001000b
#define ADS1115_ADDR_VDD    0x49 // 1001001b
#define ADS1115_ADDR_SDA    0x4A // 1001010b
#define ADS1115_ADDR_SCL    0x4B // 1001011b

/* Register Pointer */
#define ADS1115_REG_CONVERSION    0x00 // Resultado da conversão
#define ADS1115_REG_CONFIG        0x01 // Configurações do dispositivo
#define ADS1115_REG_LO_THRESH     0x02 // Limiar inferior do comparador
#define ADS1115_REG_HI_THRESH     0x03 // Limiar superior do comparador

/* Config Register */
#define ADS1115_OS_START          (1 << 15) // Inicia conversão (single-shot mode)
#define ADS1115_OS_BUSY           (0 << 15) // ADC está convertendo
#define ADS1115_OS_READY          (1 << 15) // ADC está ocioso

/* Input MUX */ 
#define ADS1115_MUX_DIFF_0_1      (0x0 << 12) // Diferencial AIN0 - AIN1 (Padrão)
#define ADS1115_MUX_DIFF_0_3      (0x1 << 12) // Diferencial AIN0 - AIN3
#define ADS1115_MUX_DIFF_1_3      (0x2 << 12) // Diferencial AIN1 - AIN3
#define ADS1115_MUX_DIFF_2_3      (0x3 << 12) // Diferencial AIN2 - AIN3
#define ADS1115_MUX_SINGLE_0      (0x4 << 12) // Single-ended AIN0
#define ADS1115_MUX_SINGLE_1      (0x5 << 12) // Single-ended AIN1
#define ADS1115_MUX_SINGLE_2      (0x6 << 12) // Single-ended AIN2
#define ADS1115_MUX_SINGLE_3      (0x7 << 12) // Single-ended AIN3

/* Programmable Gain Amplifier */
#define ADS1115_PGA_6_144V        (0x0 << 9)  // +/- 6.144V
#define ADS1115_PGA_4_096V        (0x1 << 9)  // +/- 4.096V
#define ADS1115_PGA_2_048V        (0x2 << 9)  // +/- 2.048V (Padrão)
#define ADS1115_PGA_1_024V        (0x3 << 9)  // +/- 1.024V
#define ADS1115_PGA_0_512V        (0x4 << 9)  // +/- 0.512V
#define ADS1115_PGA_0_256V        (0x5 << 9)  // +/- 0.256V

/* Mode operation */
#define ADS1115_MODE_CONTINUOUS   (0 << 8)    // Conversão Contínua
#define ADS1115_MODE_SINGLE_SHOT  (1 << 8)    // Power-down / Single-shot (Padrão)

/* Data Rate */
#define ADS1115_DR_8SPS           (0x0 << 5)
#define ADS1115_DR_16SPS          (0x1 << 5)
#define ADS1115_DR_32SPS          (0x2 << 5)
#define ADS1115_DR_64SPS          (0x3 << 5)
#define ADS1115_DR_128SPS         (0x4 << 5)  // Padrão
#define ADS1115_DR_250SPS         (0x5 << 5)
#define ADS1115_DR_475SPS         (0x6 << 5)
#define ADS1115_DR_860SPS         (0x7 << 5)

/* Comparator Mode */
#define ADS1115_COMP_MODE_TRAD    (0 << 4)    // Tradicional com histerese
#define ADS1115_COMP_MODE_WINDOW  (1 << 4)    // Janela

#define ADS1115_COMP_POL_LOW      (0 << 3)    // Ativo Baixo
#define ADS1115_COMP_POL_HIGH     (1 << 3)    // Ativo Alto

#define ADS1115_COMP_LAT_NON      (0 << 2)    // Sem trava
#define ADS1115_COMP_LAT_LATCH    (1 << 2)    // Com trava

#define ADS1115_COMP_QUE_1CONV    (0x0)       // Assert após 1 conversão
#define ADS1115_COMP_QUE_2CONV    (0x1)       // Assert após 2 conversões
#define ADS1115_COMP_QUE_4CONV    (0x2)       // Assert após 4 conversões
#define ADS1115_COMP_QUE_DISABLE  (0x3)       // Desabilita comparador (Padrão)


class HalAdc {
    public:
        enum class AdcChannel {
            AIN0,
            AIN1,
            AIN2,
            AIN3
        };

        HalAdc(HalI2C& i2c, HalGpio* alert_gpio = nullptr);

        bool configure(uint16_t config);
        bool start_conversion();

        bool read_raw(int16_t& value);
        float read_voltage();
        float read(AdcChannel ch);

    private:
        bool wait_conversion_ready(int timeout_ms);
        bool setup_ready_pin(void);
        float pga_to_lsb() const;
        uint16_t channel_to_mux(AdcChannel ch) const;

        HalI2C& _i2c;
        HalGpio* _alert{nullptr};
        uint16_t _config{0};
};

#endif
