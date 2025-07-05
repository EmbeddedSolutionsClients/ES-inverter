/*
 * Copyright (C) 2024 EmbeddedSolutions.pl
 */

#ifndef EVT_H_
#define EVT_H_

void sys_evt_init(void);
void led_evt_init(void);

inline void evt_init(void)
{
  sys_evt_init();
  led_evt_init();
}

esp_err_t led_evt_identify_msg_handler(void *data);

#endif /* EVT_H_ */
