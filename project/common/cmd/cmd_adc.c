/*
 * Copyright (C) 2017 XRADIO TECHNOLOGY CO., LTD. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the
 *       distribution.
 *    3. Neither the name of XRADIO TECHNOLOGY CO., LTD. nor the names of
 *       its contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "cmd_util.h"
#include "cmd_adc.h"
#include "driver/chip/hal_gpio.h"
#include "driver/chip/hal_adc.h"

#define CMD_ADC_TEST_DBG	0

static uint8_t cmd_adc_pass;

static struct cmd_adc_test_priv{
	uint8_t		enable;
	uint8_t		irq_mode;
	uint32_t	low_value;
	uint32_t	high_value;
	uint32_t	voltage;
	uint32_t	deviation;
} priv[ADC_CHANNEL_NUM];

static struct cmd_adc_common_priv{
	uint8_t		work_mode;
	uint32_t	voltage;
	uint32_t	deviation;
} common_priv;

static enum cmd_status cmd_adc_output_check(uint32_t data, uint32_t voltage,
											uint32_t deviation)
{
	uint32_t voltage_low, voltage_high;

	voltage_low = (voltage < deviation) ? 0 : (voltage - deviation);
	voltage_high = ((voltage + deviation) > 2500) ? 2500 : (voltage + deviation);

	voltage_low = voltage_low * 4096 / 2500;
	voltage_high = voltage_high * 4096 / 2500;

	if ((data >= voltage_low) && (data <= voltage_high)) {
#if CMD_ADC_TEST_DBG
		CMD_DBG("ADC: data = %u, voltage_low = %u, voltage_high = %u\r\n",
				data, voltage_low, voltage_high);
#endif /* CMD_ADC_TEST_DBG */
		return CMD_STATUS_OK;
	} else {
		CMD_ERR("ADC: data = %u, voltage_low = %u, voltage_high = %u\r\n",
				data, voltage_low, voltage_high);
		return CMD_STATUS_FAIL;
	}
}

static void cmd_adc_irq_cb(void *arg)
{
	uint32_t channel, data;
	ADC_IRQState irq_state;

	if (common_priv.work_mode == ADC_BURST_CONV) {
		uint8_t i, num;
		num = HAL_ADC_GetFifoDataCount();
		for(i = 0; i <= num; i++) {
			if (CMD_STATUS_OK != cmd_adc_output_check(HAL_ADC_GetFifoData(),
							common_priv.voltage, common_priv.deviation))
				break;
		}
		return;
	}

	for (channel = ADC_CHANNEL_0; channel < ADC_CHANNEL_NUM; channel++) {
		if (priv[channel].enable == 0)
			continue;

		cmd_adc_pass = 1;

		data		= HAL_ADC_GetValue((ADC_Channel)channel);
		irq_state	= HAL_ADC_GetIRQState((ADC_Channel)channel);

#if CMD_ADC_TEST_DBG
		CMD_DBG("channel = %u, irq_mode = %u, irq_state = %d, low_value = %u, high_value = %u, data = %u\r\n",
				channel, priv[channel].irq_mode, irq_state, priv[channel].low_value,
				priv[channel].high_value, data);
#endif /* CMD_ADC_TEST_DBG */

		if (CMD_STATUS_OK != cmd_adc_output_check(data, priv[channel].voltage, priv[channel].deviation))
			cmd_adc_pass = 0;

		switch (priv[channel].irq_mode) {
		case ADC_IRQ_NONE:
			cmd_adc_pass = 0;
			CMD_ERR("irq_mode: ADC_IRQ_NONE, irq_state = %d\r\n", irq_state);
			break;
		case ADC_IRQ_DATA:
			if (irq_state != ADC_DATA_IRQ) {
				cmd_adc_pass = 0;
				CMD_ERR("irq_mode: ADC_IRQ_DATA, irq_state = %d\r\n", irq_state);
			}
			break;
		case ADC_IRQ_LOW:
			if ((irq_state != ADC_LOW_IRQ) || (data > priv[channel].low_value)) {
				cmd_adc_pass = 0;
				CMD_ERR("irq_mode: ADC_IRQ_LOW, irq_state = %d, low_value = %u, data = %u\r\n",
						irq_state, priv[channel].low_value, data);
			}
			break;
		case ADC_IRQ_HIGH:
			if ((irq_state != ADC_HIGH_IRQ) || (data < priv[channel].high_value)) {
				cmd_adc_pass = 0;
				CMD_ERR("irq_mode: ADC_IRQ_HIGH, irq_state = %d, high_value = %u, data = %u\r\n",
						irq_state, priv[channel].high_value, data);
			}
			break;
		case ADC_IRQ_LOW_DATA:
			if (irq_state != ADC_LOW_DATA_IRQ && irq_state != ADC_DATA_IRQ) {
				cmd_adc_pass = 0;
				CMD_ERR("irq_mode: ADC_IRQ_LOW_DATA, irq_state = %d\r\n", irq_state);
			}
			break;
		case ADC_IRQ_HIGH_DATA:
			if (irq_state != ADC_HIGH_DATA_IRQ && irq_state != ADC_DATA_IRQ) {
				cmd_adc_pass = 0;
				CMD_ERR("irq_mode: ADC_IRQ_HIGH_DATA, irq_state = %d\r\n", irq_state);
			}
			break;
		case ADC_IRQ_LOW_HIGH:
			if (irq_state != ADC_LOW_IRQ && irq_state != ADC_HIGH_IRQ) {
				cmd_adc_pass = 0;
				CMD_ERR("irq_mode: ADC_IRQ_LOW_HIGH, irq_state = %d\r\n", irq_state);
			}
			break;
		case ADC_IRQ_LOW_HIGH_DATA:
			if (irq_state != ADC_LOW_DATA_IRQ && irq_state != ADC_HIGH_DATA_IRQ
			                                  && irq_state != ADC_DATA_IRQ) {
				cmd_adc_pass = 0;
				CMD_ERR("irq_mode: ADC_IRQ_LOW_HIGH_DATA, irq_state = %d\r\n", irq_state);
			}
			break;
		default:
			cmd_adc_pass = 0;
			CMD_ERR("channel = %u, irq_mode = %u\r\n", channel, priv[channel].irq_mode);
			break;
		}
	}

	return;
}

static enum cmd_status cmd_adc_init_exec(char *cmd)
{
	int cnt;
	uint32_t freq;
	uint32_t delay;
	uint32_t work_mode;
	ADC_InitParam adc_param;
	HAL_Status hal_status;

	cnt = cmd_sscanf(cmd, "m=%u f=%u d=%u", &work_mode, &freq, &delay);
	if (cnt != 3) {
		CMD_ERR("cmd_sscanf return: cnt = %d\r\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}

	if (work_mode > 1) {
		CMD_ERR("invalid work mode %u\r\n", work_mode);
		return CMD_STATUS_INVALID_ARG;
	}

	if ((freq < 1000) || (freq > 1000000)) {
		CMD_ERR("invalid freq %u\r\n", freq);
		return CMD_STATUS_INVALID_ARG;
	}

	if ((delay >> 8) != 0) {
		CMD_ERR("invalid delay %u\r\n", delay);
		return CMD_STATUS_INVALID_ARG;
	}

	cmd_memset(priv, 0, ADC_CHANNEL_NUM * sizeof(struct cmd_adc_test_priv));

	adc_param.freq		= freq;
	adc_param.delay		= delay;
	adc_param.mode		= work_mode ? ADC_BURST_CONV : ADC_CONTI_CONV;
#if (__CONFIG_CHIP_ARCH_VER == 2)
	adc_param.vref_mode = ADC_VREF_MODE_1;
#endif

	hal_status = HAL_ADC_Init(&adc_param);
	if (hal_status == HAL_OK) {
		common_priv.work_mode = adc_param.mode;
		return CMD_STATUS_OK;
	} else {
		CMD_ERR("HAL_ADC_Init return: hal_status = %d\r\n", hal_status);
		return CMD_STATUS_FAIL;
	}
}

static enum cmd_status cmd_adc_deinit_exec(char *cmd)
{
	HAL_Status hal_status;

	hal_status = HAL_ADC_DeInit();
	if (hal_status == HAL_OK) {
		return CMD_STATUS_OK;
	} else {
		CMD_ERR("HAL_ADC_DeInit return: hal_status = %d\r\n", hal_status);
		return CMD_STATUS_FAIL;
	}
}

static enum cmd_status cmd_adc_conv_polling_exec(char *cmd)
{
	int cnt;
	uint32_t channel, voltage, deviation;
	uint32_t data;
	HAL_Status hal_status;

	cnt = cmd_sscanf(cmd, "c=%u v=%u d=%u", &channel, &voltage, &deviation);
	if (cnt != 3) {
		CMD_ERR("cmd_sscanf return: cnt = %d\r\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}

	if (channel >= ADC_CHANNEL_NUM) {
		CMD_ERR("invalid channel %u\r\n", channel);
		return CMD_STATUS_INVALID_ARG;
	}

	if (voltage > 2500) {
		CMD_ERR("invalid voltage %u\r\n", voltage);
		return CMD_STATUS_INVALID_ARG;
	}

	if (deviation > 2500) {
		CMD_ERR("invalid deviation %u\r\n", deviation);
		return CMD_STATUS_INVALID_ARG;
	}

	hal_status = HAL_ADC_Conv_Polling((ADC_Channel)channel, &data, 10000);
	if (hal_status != HAL_OK) {
		CMD_ERR("HAL_ADC_Conv_Polling return: hal_status = %d\r\n", hal_status);
		return CMD_STATUS_FAIL;
	}

	return cmd_adc_output_check(data, voltage, deviation);
}

static enum cmd_status cmd_adc_config_exec(char *cmd)
{
	int cnt;
	uint32_t channel, enable, irq_mode, low_value, high_value, voltage, deviation;
	HAL_Status hal_status;

	cnt = cmd_sscanf(cmd, "c=%u e=%u i=%u l=%u h=%u v=%u d=%u",
					 &channel, &enable, &irq_mode, &low_value,
					 &high_value, &voltage, &deviation);
	if (cnt != 7) {
		CMD_ERR("cmd_sscanf return: cnt = %d\r\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}

	if (channel >= ADC_CHANNEL_NUM) {
		CMD_ERR("invalid channel %u\r\n", channel);
		return CMD_STATUS_INVALID_ARG;
	}

	if (enable > 1) {
		CMD_ERR("invalid enable %u\r\n", enable);
		return CMD_STATUS_INVALID_ARG;
	}

	if (irq_mode > 7) {
		CMD_ERR("invalid irq_mode %u\r\n", irq_mode);
		return CMD_STATUS_INVALID_ARG;
	}

	if ((low_value >> 12) != 0) {
		CMD_ERR("invalid low_value %u\r\n", low_value);
		return CMD_STATUS_INVALID_ARG;
	}

	if ((high_value >> 12) != 0) {
		CMD_ERR("invalid high_value %u\r\n", high_value);
		return CMD_STATUS_INVALID_ARG;
	}

	if (voltage > 2500) {
		CMD_ERR("invalid voltage %u\r\n", voltage);
		return CMD_STATUS_INVALID_ARG;
	}

	if (deviation > 2500) {
		CMD_ERR("invalid deviation %u\r\n", deviation);
		return CMD_STATUS_INVALID_ARG;
	}

	if (common_priv.work_mode == ADC_BURST_CONV) {
		hal_status = HAL_ADC_FifoConfigChannel((ADC_Channel)channel, (ADC_Select)enable);
		if (hal_status != HAL_OK) {
			CMD_ERR("HAL_ADC_FifoConfigChannel return: hal_status = %d\r\n", hal_status);
			return CMD_STATUS_FAIL;
		}
		common_priv.voltage = voltage;
		common_priv.deviation = deviation;

		return CMD_STATUS_OK;
	}

	hal_status = HAL_ADC_ConfigChannel((ADC_Channel)channel, (ADC_Select)enable,
									   (ADC_IRQMode)irq_mode, low_value, high_value);
	if (hal_status != HAL_OK) {
		CMD_ERR("HAL_ADC_ConfigChannel return: hal_status = %d\r\n", hal_status);
		return CMD_STATUS_FAIL;
	}

	priv[channel].enable		= (uint8_t)enable;
	priv[channel].irq_mode		= (uint8_t)irq_mode;
	priv[channel].low_value		= low_value;
	priv[channel].high_value	= high_value;
	priv[channel].voltage		= voltage;
	priv[channel].deviation		= deviation;

#if CMD_ADC_TEST_DBG
	CMD_DBG("ADC config: channel = %u, enable = %u, irq_mode = %u, low_value = %u, high_value = %u, voltage = %u, deviation = %u\r\n",
			channel, enable, irq_mode, low_value, high_value, voltage, deviation);
#endif /* CMD_ADC_TEST_DBG */

	return CMD_STATUS_OK;
}

static enum cmd_status cmd_adc_conv_it_start_exec(char *cmd)
{
	ADC_Channel chan;
	HAL_Status hal_status;

	cmd_adc_pass = 1;

	for (chan = ADC_CHANNEL_0; chan < ADC_CHANNEL_NUM; chan++) {
		hal_status = HAL_ADC_EnableIRQCallback(chan, cmd_adc_irq_cb, NULL);
		if (hal_status != HAL_OK) {
			CMD_ERR("HAL_ADC_EnableIRQCallback return: hal_status = %d\r\n", hal_status);
			return CMD_STATUS_FAIL;
		}
	}

	hal_status = HAL_ADC_Start_Conv_IT();
	if (hal_status != HAL_OK) {
		CMD_ERR("HAL_ADC_Start_Conv_IT return: hal_status = %d\r\n", hal_status);
		return CMD_STATUS_FAIL;
	}

	return CMD_STATUS_OK;
}

static enum cmd_status cmd_adc_conv_it_stop_exec(char *cmd)
{
	ADC_Channel chan;
	HAL_Status hal_status;

	hal_status = HAL_ADC_Stop_Conv_IT();
	if (hal_status != HAL_OK) {
		CMD_ERR("HAL_ADC_Stop_Conv_IT return: hal_status = %d\r\n", hal_status);
		return CMD_STATUS_FAIL;
	}

	for (chan = ADC_CHANNEL_0; chan < ADC_CHANNEL_NUM; chan++) {
		hal_status = HAL_ADC_DisableIRQCallback(chan);
		if (hal_status != HAL_OK) {
			CMD_ERR("HAL_ADC_DisableIRQCallback return: hal_status = %d\r\n", hal_status);
			return CMD_STATUS_FAIL;
		}
	}

	if (cmd_adc_pass == 1)
		return CMD_STATUS_OK;
	else
		return CMD_STATUS_FAIL;
}

static const struct cmd_data g_adc_cmds[] = {
    { "init",           cmd_adc_init_exec },
    { "deinit",         cmd_adc_deinit_exec },
    { "conv-polling",   cmd_adc_conv_polling_exec },
    { "config",         cmd_adc_config_exec },
    { "conv-it-start",  cmd_adc_conv_it_start_exec },
    { "conv-it-stop",   cmd_adc_conv_it_stop_exec },
};

enum cmd_status cmd_adc_exec(char *cmd)
{
	return cmd_exec(cmd, g_adc_cmds, cmd_nitems(g_adc_cmds));
}
