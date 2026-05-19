#include "board_manage.h"
// #include "ota_service_task.h"
#include "at_protocol_handler.h"

uint8_t craner_at_handler(const uint8_t *buf, uint16_t len)
{
	static const char at_prefix[] = "craner#AT";
	const uint16_t at_prefix_len = (uint16_t)(sizeof(at_prefix) - 1U);
	uint16_t index = 0U;
	char tmp[256];
	uint16_t tlen;

	/* Skip leading whitespace */
	while (index < len)
	{
		if ((buf[index] != ' ') && (buf[index] != '\t') && (buf[index] != '\r') && (buf[index] != '\n'))
		{
			break;
		}
		index++;
	}

	/* Check if remaining length is sufficient */
	if ((len - index) < at_prefix_len)
	{
		return 0U;
	}

	/* Compare prefix */
	if (memcmp(&buf[index], at_prefix, at_prefix_len) == 0)
	{
		/* Convert to string for easier parsing */
		tlen = (len > 255U) ? 255U : len;
		memcpy(tmp, buf, tlen);
		tmp[tlen] = '\0';

		/* Handle VERSION query command */
		// if (strstr(tmp, "craner#AT+SYSVER") != NULL)
		// {
		// 	char ver_buf[64];
		// 	const system_info_t *info = board_get_system_info();
		// 	snprintf(ver_buf, sizeof(ver_buf), "VERSION=%s;BUILD=%s;\r\n",
		// 	         info->name, info->build_timestamp);
		// 	(void)uart_manage_dma_send_by_name("shell", (uint8_t *)ver_buf,
		// 	                                   (uint16_t)strlen(ver_buf));
		// 	return 1U;
		// }
		
		if (strstr(tmp, "craner#AT") != NULL)
		{
			const char ok[] = "craner#OK\r\n";
			(void)uart_manage_dma_send_by_name("shell", (uint8_t *)ok, (uint16_t)(sizeof(ok) - 1U));
			return 1U;
		}

		/* Handle OTA START command */
		// if (strstr(tmp, "craner#AT+OTASTART") != NULL)
		// {
		// 	int ret = ota_start_transfer();
		// 	if (ret == 0)
		// 	{
		// 		const char ack[] = "craner#OK\r\n";
		// 		(void)uart_manage_dma_send_by_name("shell", (uint8_t *)ack, (uint16_t)(sizeof(ack) - 1U));
		// 	}
		// 	else
		// 	{
		// 		const char err[] = "craner#ERROR\r\n";
		// 		(void)uart_manage_dma_send_by_name("shell", (uint8_t *)err, (uint16_t)(sizeof(err) - 1U));
		// 	}
		// 	return 1U;
		// }

		/* Unknown command */
		{
			const char err[] = "craner#UNKNOWN\r\n";
			(void)uart_manage_dma_send_by_name("shell", (uint8_t *)err, (uint16_t)(sizeof(err) - 1U));
		}

		return 1U;
	}

	return 0U;
}

uint8_t usr_at_handler(const uint8_t *buf, uint16_t len)
{
	static const char at_prefix[] = "usr.cn#AT";
	const uint16_t at_prefix_len = (uint16_t)(sizeof(at_prefix) - 1U);
	uint16_t index = 0U;

	while (index < len)
	{
		if ((buf[index] != ' ') && (buf[index] != '\t') && (buf[index] != '\r') && (buf[index] != '\n'))
		{
			break;
		}
		index++;
	}

	if ((len - index) < at_prefix_len)
	{
		return 0U;
	}

	if (memcmp(&buf[index], at_prefix, at_prefix_len) == 0)
	{
		/* Forward the original payload to the 4G module first. */
		(void)uart_manage_dma_send_by_name("4g", buf, len);

		/* Add trailing CRLF if the command does not already end with it. */
		if ((len > 0U) && (buf[len - 1U] != '\r') && (buf[len - 1U] != '\n'))
		{
			static const uint8_t crlf[] = "\r\n";
			const uint16_t crlf_len = (uint16_t)(sizeof(crlf) - 1U);
			(void)uart_manage_dma_send_by_name("4g", (uint8_t *)crlf, crlf_len);
		}

		return 1U;
	}

	return 0U;
}

