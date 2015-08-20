# RM + Slack Stealing scheduler

Implementación de la política de planificación Rate Monotonic (RM) con Slack
Stealing (SS) para la administración del tiempo ocioso.

Si se asigna 1 a USE_SLACK_K, el calculo de slack solo se realiza al iniciar el
planificador, y este valor es vuelto a cargar en cada fin de instancia.
