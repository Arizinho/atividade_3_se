# Semáforo com Feedback para Pessoas com Deficiência Visual

Este projeto foi desenvolvido utilizando a placa BitDogLab. Tem como objetivo simular um semáforo de pedestres com sinalização acessível, por meio de luzes e sons, para pessoas com deficiência visual.

O sistema funciona em dois modos:
- **Modo Normal:** alterna automaticamente entre os sinais vermelho, verde e amarelo. Cada estado possui uma luz correspondente no LED RGB, um som característico no buzzer e uma mensagem no display OLED.
- **Modo Noturno:** ativado com o botão A, pisca luz amarela lentamente e emite beeps espaçados.

Todas as funcionalidades são controladas por tarefas do FreeRTOS.
