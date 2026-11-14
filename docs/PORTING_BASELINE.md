# PCSX2-360 — baseline de integração

Data da auditoria: 2026-11-14  
Repositório de destino: https://github.com/MeiKinosaki/PCSX2-360  
Base PS2 auxiliar: https://github.com/MeiKinosaki/PS2Palace360

## Estado inicial verificado

- Branch de destino: `main`.
- O repositório PCSX2-360 contém apenas o README inicial e o arquivo LICENSE; ainda não há código-fonte, workflow ou artefato.
- O README inicial contém somente o título do projeto.
- A licença atualmente presente no destino é MPL-2.0.
- O PS2Palace360 usa licença MIT.
- O PCSX2 upstream declara GPL-3.0. Antes de importar arquivos, registrar origem, copyright e licença por arquivo e confirmar as condições de compatibilidade/distribuição. Não remover avisos de licença.

## Inventário PS2Palace360 verificado

### Código EE existente

- `src/ps2/ee_state.c` / `ee_state.h`: estado-base experimental do Emotion Engine.
- `src/ps2/ee_ops.c` / `ee_ops.h`: operações escalares já isoladas.
- `src/ps2/ee_immediate.c` / `ee_immediate.h`: operações imediatas.
- `src/ps2/ee_compare.c` / `ee_compare.h`: comparações.
- `src/ps2/ee_decode.c` / `ee_decode.h`: decodificação parcial.
- `src/ps2/ee_memory.c` / `ee_memory.h`: memória experimental, tradução direta KSEG0/KSEG1 e scaffold TLB limitado.
- `src/ps2/ee_timer.c` / `ee_timer.h`: temporizador EE experimental, ainda não fiel ao hardware nem integrado ao scheduler/CP0.
- `src/ps2/ee_dma.c` / `ee_dma.h`: cópia genérica com limites e sobreposição segura; não equivale aos dez canais DMA reais do PS2.
- `src/main.c`: bootstrap placeholder, não é runtime Xbox funcional.

### Testes e automação existentes

- `tests/test_ee_state.c`
- `tests/test_ee_timer.c`
- `tests/test_ee_dma.c`
- `.github/workflows/host-validation.yml`
- `.github/workflows/cloud-toolchain.yml`
- `Makefile` com validação de host e alvo de XEX condicionado à presença de `xenon-gcc` e `elf2xex`.

O histórico do PS2Palace360 registra regressão host aprovada para EE/timer/DMA. A toolchain Xenon não estava disponível no runner registrado e nenhum `default.xex` funcional foi comprovado.

## Política de migração

1. Preservar PS2Palace360 sem alterações destrutivas.
2. Importar para PCSX2-360 somente módulos identificados, com seus testes e atribuição MIT.
3. Tratar os módulos EE existentes como protótipos auxiliares, não como implementação completa do PS2.
4. Auditar dependências, licenças e arquitetura antes de importar código do PCSX2 upstream.
5. Manter o núcleo do emulador separado do backend Xbox 360: CPU/memória/dispositivos não devem depender diretamente de APIs de vídeo/entrada do console.
6. Não copiar o sistema de build desktop inteiro do PCSX2; criar uma camada de plataforma Xenon e adaptar dependências uma a uma.
7. Preservar `default.xex` como artefato final solicitado. ELF, se necessário, será intermediário.
8. Não declarar teste, build, boot ou compatibilidade sem log/artefato/evidência correspondente.

## Ordem de integração proposta

1. Reconciliar o inventário de licenças e dependências.
2. Trazer os testes host e módulos EE independentes do PS2Palace360, mantendo atribuição e histórico de origem.
3. Executar a suíte host no PCSX2-360 e corrigir diferenças de compilação.
4. Comparar o EE atual com a semântica R5900 do PCSX2; priorizar decodificação, exceções, delay slots, CP0 e precisão de memória.
5. Definir interfaces para IOP, DMA real, VU0/VU1, GIF, GS, SPU2 e CDVD.
6. Criar/adaptar o backend gráfico para Xenos e o runtime de entrada/áudio do Xbox 360.
7. Configurar runner reproduzível com toolchain Xenon/libXenon e conversor XEX.
8. Gerar e validar `default.xex`; depois testar inicialização no Xbox 360 RGH/JTAG.

## Critérios de status

- Planejado ≠ implementado.
- Código presente ≠ integrado.
- Teste escrito ≠ teste aprovado.
- Teste host aprovado ≠ build Xenon aprovado.
- XEX gerado ≠ XEX inicializado no console.
- XEX inicializado ≠ jogo PS2 compatível.
- A versão só avança quando o escopo daquela versão estiver concluído e as evidências forem registradas.


## Atualização após a primeira integração no PCSX2-360

Em 14/11/2026 foram importados para `src/ps2/` os módulos EE `ee_state`, `ee_ops`, `ee_immediate`, `ee_compare`, `ee_decode` e `ee_memory`, além de `tests/test_ee_state.c`. O aviso de licença MIT da base PS2Palace360 foi preservado em `LICENSES/PS2Palace360-MIT.txt`.

Foi criado um `Makefile` com validação host e alvo `make xex`. O artefato de entrega é exclusivamente `build/default.xex`; um ELF intermediário temporário é removido após a conversão. O workflow `.github/workflows/build.yml` executa validação host no GitHub-hosted runner e reserva a compilação Xenon para runner self-hosted com libXenon/`elf2xex` configurados.

A importação ainda não foi validada por CI neste repositório e nenhum `default.xex` foi gerado. `src/main.c` é somente um bootstrap experimental; não implementa inicialização do Xbox 360 nem boot de jogos PS2.
