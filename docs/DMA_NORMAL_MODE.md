# EE DMA: primeiro passo do motor de transferência

Estado: implementação experimental parcial; não é ainda um controlador DMA EE completo.

## Escopo desta etapa

- Mantém os 10 slots de canal existentes e os registradores CHCR/MADR/QWC/TADR.
- Implementa `ps2_dma_step`, que processa no máximo um QWC (16 bytes) por chamada.
- Só aceita o modo normal (CHCR.MOD = 0); outros modos retornam erro sem avançar o canal.
- Verifica alinhamento de MADR em 16 bytes e overflow antes de chamar o endpoint.
- Encaminha canal, endereço e CHCR.DIR para um callback de dispositivo.
- Só atualiza MADR/QWC depois de callback bem-sucedido; limpa STR ao concluir QWC.
- Testes cobrem conclusão de duas unidades, direção, callback com falha, modo não suportado e endereço desalinhado.

## Limitações explícitas

O callback representa apenas o ponto de integração com um dispositivo. Esta etapa ainda não lê/escreve a RAM do EE nem implementa FIFOs reais. Não há arbitragem entre canais, DPCR/DICR funcionais, geração de IRQ, chain mode, tags, TTE/TIE, scratchpad, VIF, GIF, SIF ou integração com scheduler. A semântica completa de cada canal EE precisa ser implementada e validada contra documentação R5900/Emotion Engine.

## Validação

O teste está incluído em `make check` pelo alvo `test_dma`. A consulta de status do commit de testes não retornou verificações; portanto, não declarar `make check` aprovado até que o GitHub Actions execute o workflow.
