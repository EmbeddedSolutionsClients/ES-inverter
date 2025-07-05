# Copyright (C) 2025 EmbeddedSolutions.pl
# Errors to investigate #

# Relay & Relay Svc

```
// At about Tue Oct 29 00:06:25 2024 that request is valid in implemented logic.
I (600807) tcp_client: Send EnergyPriceDayGet
I (600807) tcp_client: TX msg type 0x42
I (600807) utils: Next day data request: Datetime: Tue Oct 29 16:06:25 2024
// No response from server but that is fine.

// At about ~Tue Oct 29 04:00:00 2024 there is another request but that one is invalid. Because log above points that next request
// should be called at Datetime: Tue Oct 29 16:06:25 2024.
I (14675127) tcp_client: Send EnergyPriceDayGet
I (14675127) tcp_client: TX msg type 0x42
I (14675127) utils: Next day data request: Datetime: Tue Oct 29 19:18:08 2024
// No response from server but that is fine.

// At about ~ Tue Oct 29 07:00:25 2024 there is another request but that one is invalid. Because log above points that next request
// should be called at Datetime: Tue Oct 29 19:18:08 2024
I (26178447) tcp_client: Send EnergyPriceDayGet
I (26178447) tcp_client: TX msg type 0x42
I (26178447) utils: Next day data request: Datetime: Tue Oct 29 15:13:05 2024
```

Predictions:
- Scheduler has problems with scheduling long delay callbacks.
- Rescheduling does not work as expected.
- Unknown issue with RelaySvc logic.

More logs are required to determine the issue.


# WIFI
Why after startup when connecting to wifi appears disconnected reason 202 & 205 before successfully connects ?
