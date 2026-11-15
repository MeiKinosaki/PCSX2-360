# DMA EE — transferência experimental com RAM

Estado: implementação parcial de modo normal, ainda sem precisão completa do controlador DMA do PS2.

## Implementado nesta etapa

- `ps2_dma_step_memory` processa no máximo um QWC (16 bytes) por chamada.
- DIR=1 copia um QWC da RAM EE para um callback de dispositivo.
- DIR=0 permite ao callback preencher um QWC, que só então é copiado para a RAM EE.
- Valida CHCR.MOD, alinhamento de 16 bytes, limites do buffer e overflow de endereço de 32 bits.
- MADR/QWC só avançam após callback bem-sucedido; STR é limpo quando QWC chega a zero.
- Testes cobrem os dois sentidos, progresso, falha de callback e acesso fora dos limites.

## Limitações

A RAM é um buffer linear fornecido pelo chamador; não representa ainda o mapa físico completo MEM1/MEM2 nem aliases. O callback não é um FIFO real de VIF/GIF/SIF/SPU2/CDVD. Não há chain mode, tags, arbitragem, prioridades DPCR, IRQ DICR, scratchpad ou integração com scheduler. A semântica por canal e os registradores ainda precisam de validação diferencial com documentação técnica do PS2.

## Validação

Os testes estão incluídos no alvo `make check`. O commit atual ainda não apresenta verificações de CI; não declarar aprovação até o workflow executar e passar.
