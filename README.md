# PCSX2-360

Projeto experimental de emulação de PlayStation 2 para Xbox 360 RGH/JTAG.

**Versão de desenvolvimento: v0.4.8b — primeira build XEX com XSDK em execução no GitHub Actions.**

**Progresso global estimado: 34%.** Marco em andamento: o fluxo foi migrado para o XSDK `xenosworks/xsdk`, fixado no commit `ade2c81d914634834d77394c5d9f8c4575f395bc`. O GitHub Actions agora usa runner hospedado `ubuntu-latest`, compila a toolchain PowerPC própria do XSDK, compila `libxsdk.a`, compila os módulos EE/IOP e chama `tools/elf2xex.py` do XSDK para criar um XEX2 de título para o kernel retail/RGH. O Makefile entrega somente `build/default.xex` e remove o ELF intermediário. Execução nº 319 (`43869392104`) foi criada automaticamente, mas ainda está `queued`; não há artefato nem build aprovada. A etapa ainda é experimental e não comprova boot no console. Marco v0.4.7b: alvo `make xex` endurecido para manter o ELF apenas como arquivo temporário, removendo ELF/XEX temporários em caso de falha e removendo o ELF após conversão bem-sucedida. Alteração registrada no commit `7602b4a2fb23005e4dec8e58190b0c4b31014b45`. Isso conclui uma melhoria do fluxo de empacotamento no código, mas não comprova a disponibilidade do conversor, compilação Xenon, geração de `default.xex` nem execução no console. Marco v0.4b: serialização portátil little-endian v2 do estado EE, TLB de 48 entradas e RAM, com checksum e validação antes da restauração; o tamanho do registro TLB foi corrigido para 25 bytes e a validação de terminadores NUL da biblioteca foi corrigida. As duas alterações foram relidas no GitHub, mas ainda aguardam execução confirmada no CI. O avanço não representa uma máquina PS2 completa. Marco anterior v0.3b: integração experimental de CP0/TLB e PageMask variável; CP0.Context, BadVAddr, Context.BadVPN2 e EntryHi.VPN2 nas exceções TLB; distinção entre TLB miss e entrada inválida; rastreamento do branch proprietário do delay slot, EPC/Cause.BD em exceções e retomada no destino do desvio. A regressão host `make check` foi aprovada no GitHub Actions para o commit `a70db3a1bc87694124747736eba7bc63e1fbaa4a` (run `43644957989`). O resultado valida apenas os testes host incluídos, não a precisão integral do R5900 nem a compatibilidade com jogos.


## Objetivo do build

O executável final obrigatório é **`build/default.xex`**. O arquivo ELF, quando exigido pelo conversor `elf2xex`, é criado apenas como intermediário temporário durante o build e removido após a conversão. Não é artefato de entrega.

## Estado atual

- [x] Repositório e baseline de migração documentados.
- [x] Primeiros módulos experimentais EE importados do PS2Palace360.
- [x] Teste host EE importado.
- [x] Aviso de licença MIT original preservado em `LICENSES/PS2Palace360-MIT.txt`.
- [x] Makefile inicial com alvo `make xex` que entrega `build/default.xex`.
- [x] Workflow host e build automática XEX em runner GitHub-hosted, com toolchain/packer XSDK fixados por commit (primeira execução pendente).
- [x] Primeira rota de exceções para falhas de dados do EE: TLB miss, Modified, ADEL, ADES e DBE no caminho experimental.
- [x] Testes de regressão adicionados para TLB miss em leitura, acesso desalinhado e store em página somente leitura.
- [x] Interface CP0 experimental para BadVAddr, Status, Cause e EPC.
- [x] Modelo experimental de interrupções EE: elegibilidade por Status.IE/IM, Cause.IP e bloqueio por EXL/ERL; amostragem entre instruções e adiamento durante delay slot. A validação diferencial das particularidades R5900 e as fontes reais de IRQ ainda estão pendentes.
- [x] Decodificação parcial de MFC0/MTC0 e testes de transferência/máscaras de escrita.
- [x] Tabela TLB experimental ampliada para 48 slots e teste do índice 47.
- [x] Modelo arquitetural experimental separado com 48 pares de páginas de 4 KiB (EntryHi, EntryLo0/1, ASID, V, D e G).
- [x] Operações COP0/TLB experimentais ligadas ao despachante arquitetural: TLBP, TLBR, TLBWI e TLBWR.
- [x] PageMask variável com sete codificações de tamanho de página aceitas pela implementação experimental e máscaras inválidas rejeitadas.
- [x] Estado CP0.Context (registrador 4), leitura/escrita parcial e atualização dos campos BadVPN2/VPN2 em falhas TLB; testes adicionados, aguardando validação no CI.
- [x] Exceções TLB distinguem miss real (refill vector) de entrada programada inválida (general vector); regressões de acesso de dados e de instruction fetch inválido verificam os vetores 0x80000000/0x80000180, CI pendente.
- [x] Rastreamento experimental de delay slots no step EE: branch PC, destino após slot, EPC e Cause.BD em falhas de fetch/dados; teste de falha de load no slot e alvo do branch adicionados. CI pendente.
- [x] Regressão do caminho feliz de branch tomado: uma instrução no delay slot, chegada ao alvo e execução da instrução-alvo; `make check` aprovado no CI do commit `a70db3a`.
- [x] Testes host para TLB pareada, ASID, permissões, busca sem correspondência, round-trip CP0/TLB, TLBWR e PageMask.
- [x] Regressão host `make check` aprovada no CI do commit `933a44c` (run `43626390249`).
- [ ] Auditoria diferencial completa da semântica R5900, incluindo PageMask, PFN, delay slots e exceções.
- [ ] Confirmar `make check` no CI após a serialização portátil v2; a última consulta de status não apresentou verificações.
- [x] Snapshot portátil experimental v2: estado EE + TLB arquitetural de 48 entradas + RAM serializados campo a campo em little-endian, cabeçalho versionado, tamanho e checksum; validação do TLB antes da restauração e testes de round-trip/corrupção/wire format adicionados. Execução desses novos testes no CI ainda pendente; exige RAM de mesmo tamanho e não inclui IOP/DMA/VU/GS nem persistência integrada ao runtime.
- [x] Caminho experimental unificado de fetch/load/store com TLB arquitetural: `ps2_ee_step_arch_tlb` traduz fetch e delega loads/stores ao executor arquitetural; testes cobrem aliases KSEG0, fetch/load/store, TLB miss, entrada inválida durante delay slot, LW desalinhado (ADEL), SW desalinhado (ADES), BadVAddr e preservação da RAM. Nova regressão cobre SB/SH/SW/SD e LB/LBU/LH/LHU/LW/LWU/LD, incluindo extensão de sinal/zero. Execução CI dessas regressões ainda pendente; auditoria diferencial R5900 e instruções restantes continuam abertas.
- [x] Modelo inicial de biblioteca de jogos: cadastro limitado, proteção contra serial duplicado, favoritos e filtros por título/serial/gênero.
- [x] Snapshot portátil experimental da biblioteca: formato versionado, campos em ordem fixa little-endian, checksum, validação de tamanho/strings/favoritos/seriais duplicados e validação antes de modificar o destino; regressões de round-trip e corrupção adicionadas (CI pendente).
- [ ] Toolchain XSDK configurada e `default.xex` compilado (workflow automático em andamento; build/artefato ainda não confirmados).
- [x] Barramento físico experimental EE: RAM principal de 32 MiB e Scratchpad de 16 KiB em `0x70000000`, com rejeição de endereços fora das regiões suportadas; não é ainda um mapa completo de registradores MMIO.
- [x] DMA EE experimental em modo normal integrada ao barramento físico `ps2_ee_bus_t`: `ps2_dma_step_bus` e `ps2_dma_fabric_step_bus` transferem QWC entre RAM principal/Scratchpad e callbacks de dispositivo. Teste específico cobre RAM e Scratchpad. Não implementa aliases/cache/MMIO completos.
- [x] Fabric experimental de endpoints por canal para VIF0, VIF1, GIF, IPU, SIF0/1/2 e SPR; serviço round-robin de um QWC por chamada e eventos de conclusão por canal. Callback de dispositivo ainda é interface genérica, sem FIFOs reais. Não implementa prioridades DPCR, registradores DICR nem IRQ ligada ao CP0.
- [x] Decodificador do formato de tag DMA de 128 bits (QWC/PCE/ID/IRQ/ADDR/SPR) com teste de wire format. A execução de chain mode, CALL/RET stack, NEXT/REF/REFS e TTE/TIE continua pendente; decodificar tag não equivale a executar cadeia.
- [x] Bloqueios técnicos e ordem de resolução documentados em `docs/KNOWN_BLOCKERS.md`.
- [ ] Inicialização comprovada em Xbox 360 RGH/JTAG.
- [ ] Emulação de jogos PS2.

**Importante:** os módulos EE importados são protótipos experimentais. O bootstrap atual não inicializa o hardware do Xbox 360 nem inicia jogos PS2. Mesmo após gerar o primeiro `default.xex`, ainda será necessário validar inicialização e integrar a máquina PS2 real.

## Módulos EE importados

Os arquivos em `src/ps2/` foram copiados do repositório [PS2Palace360](https://github.com/MeiKinosaki/PS2Palace360), preservando a licença MIT e sua atribuição. Incluem estado EE, operações escalares, imediatas, comparações, decodificação parcial e memória/TLB experimental.

## Build

Validação nativa:

```sh
make check
```

Build final para Xbox 360, em runner com compilador Xenon/libXenon e conversor compatível com a interface `elf2xex entrada.elf saida.xex`:

```sh
make clean
make xex
```

Saída esperada: `build/default.xex`.

O job de XEX é acionado automaticamente em pushes para `main` usando `ubuntu-latest`; baixa o XSDK no commit fixado, constrói sua toolchain e runtime e empacota o ELF PPC32 com o packer do próprio XSDK. O alvo `make xex` apaga resultados antigos antes de compilar, valida o cabeçalho `XEX2` e remove o ELF intermediário inclusive quando o empacotamento falha. A concorrência do workflow cancela execuções antigas da mesma branch para evitar uma fila de builds obsoletas. A geração de `build/default.xex` só será confirmada quando o job terminar e o upload for verificado.

## Próximas etapas

1. Executar `make check` no CI e corrigir regressões do snapshot EE, biblioteca e novo motor DMA; consultar `docs/KNOWN_BLOCKERS.md` para bloqueios e critérios de conclusão.
2. Fechar o caminho de fetch, load/store e exceções com a TLB arquitetural no executor principal.
3. Unificar `ps2_dma_step_memory` com `ps2_ee_bus_t`; implementar execução real de tags/chains (REFE/CNT/NEXT/REF/REFS/CALL/RET/END), prioridades DPCR, flags/máscaras DICR e entrega de IRQ ao CP0; substituir callbacks genéricos por FIFOs reais de VIF/GIF/SIF/IPU/SPR.
4. Desenvolver VU0/VU1, VIF, GIF, GS, SPU2 e CDVD.
5. Criar backend gráfico Xenos e runtime Xbox 360.
6. Configurar runner Xenon, gerar `build/default.xex` e testar no console.

## Regras de progresso

Código importado não significa integração completa. Testes importados não significam aprovação no novo repositório. Build host, build Xenon, geração do XEX e execução no console são marcos separados.

## DMA, dispositivos e IOP — checkpoint experimental

A etapa atual acrescenta motor incremental de tags DMA (REFE/CNT/NEXT/REF/REFS/CALL/RET/END), pilha CALL/RET de dois níveis, DPCR/DICR experimentais, entrega de conclusão DMA ao Cause.IP2, filas qword isoladas para VIF0/VIF1/GIF/SIF0/SIF1/SIF2 com adaptador DMA, subconjunto inicial de instruções inteiras IOP R3000A e scheduler EE/IOP/DMA com razão provisória 1:8. Os novos testes estão integrados ao alvo `make check`.

**Limitações importantes:** chain mode ainda não está ligado ao estado CHCR.MOD de cada canal; TTE/TIE, IRQ por tag, SPR e detalhes do DMAC aguardam validação diferencial. As filas não implementam decodificação de comandos VIF, pacotes GIF nem protocolo SIF real. O IOP cobre apenas um subconjunto de instruções e não possui exceções/COP0/timers/IRQ próprios. A ordem de eventos e razão de clock do scheduler são provisórias. O workflow push deve executar `make check`, mas ainda não há resultado de CI confirmado para estes commits. O build Xenon continua dependente de runner self-hosted; não existe XEX validado neste marco.

Detalhes: [docs/DMA_IOP_SCHEDULER_STATUS.md](docs/DMA_IOP_SCHEDULER_STATUS.md).
