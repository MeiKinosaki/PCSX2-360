# PCSX2-360

Projeto experimental de emulação de PlayStation 2 para Xbox 360 RGH/JTAG.

**Versão de desenvolvimento: v0.2b — em andamento.**

**Progresso global estimado: 20%.** Marco v0.2b: integração experimental das operações COP0/TLB ao despachante EE, PageMask variável validado por testes host e regressão aprovada no commit `933a44c`.


## Objetivo do build

O executável final obrigatório é **`build/default.xex`**. O arquivo ELF, quando exigido pelo conversor `elf2xex`, é criado apenas como intermediário temporário durante o build e removido após a conversão. Não é artefato de entrega.

## Estado atual

- [x] Repositório e baseline de migração documentados.
- [x] Primeiros módulos experimentais EE importados do PS2Palace360.
- [x] Teste host EE importado.
- [x] Aviso de licença MIT original preservado em `LICENSES/PS2Palace360-MIT.txt`.
- [x] Makefile inicial com alvo `make xex` que entrega `build/default.xex`.
- [x] Workflow de regressão host e workflow manual para runner Xenon self-hosted.
- [x] Primeira rota de exceções para falhas de dados do EE: TLB miss, Modified, ADEL, ADES e DBE no caminho experimental.
- [x] Testes de regressão adicionados para TLB miss em leitura, acesso desalinhado e store em página somente leitura.
- [x] Interface CP0 experimental para BadVAddr, Status, Cause e EPC.
- [x] Decodificação parcial de MFC0/MTC0 e testes de transferência/máscaras de escrita.
- [x] Tabela TLB experimental ampliada para 48 slots e teste do índice 47.
- [x] Modelo arquitetural experimental separado com 48 pares de páginas de 4 KiB (EntryHi, EntryLo0/1, ASID, V, D e G).
- [x] Operações COP0/TLB experimentais ligadas ao despachante arquitetural: TLBP, TLBR, TLBWI e TLBWR.
- [x] PageMask variável com sete codificações de tamanho de página aceitas pela implementação experimental e máscaras inválidas rejeitadas.
- [x] Testes host para TLB pareada, ASID, permissões, busca sem correspondência, round-trip CP0/TLB, TLBWR e PageMask.
- [x] Regressão host `make check` aprovada no CI do commit `933a44c` (run `43626390249`).
- [ ] Auditoria diferencial completa da semântica R5900, incluindo PageMask, PFN, delay slots e exceções.
- [ ] Integração de todas as instruções de memória e fetch no caminho principal da CPU.
- [ ] Toolchain Xenon configurada e `default.xex` compilado.
- [ ] Inicialização comprovada em Xbox 360 RGH/JTAG.
- [ ] Emulação de jogos PS2.

**Importante:** os módulos EE importados são protótipos experimentais. O bootstrap atual não inicializa o hardware do Xbox 360 nem inicia jogos PS2. Não há ainda um `default.xex` compilado ou testado.

## Módulos EE importados

Os arquivos em `src/ps2/` foram copiados do repositório [PS2Palace360](https://github.com/MeiKinosaki/PS2Palace360), preservando a licença MIT e sua atribuição. Incluem estado EE, operações escalares, imediatas, comparações, decodificação parcial e memória/TLB experimental.

## Build

Validação nativa:

```sh
make check
```

Build final para Xbox 360, em runner que tenha `xenon-gcc` e `elf2xex`:

```sh
make clean
make xex
```

Saída esperada: `build/default.xex`.

O job Xenon do GitHub Actions usa runner self-hosted com labels `linux` e `xenon`; um runner Ubuntu hospedado comum não inclui libXenon automaticamente.

## Próximas etapas

1. Auditar CP0/TLB por testes diferenciais contra referências R5900/PCSX2, incluindo PageMask, PFN e exceções.
2. Fechar o caminho de fetch, load/store e exceções com a TLB arquitetural no executor principal.
3. Integrar scheduler EE, IOP e os dez canais DMA reais.
4. Desenvolver VU0/VU1, VIF, GIF, GS, SPU2 e CDVD.
5. Criar backend gráfico Xenos e runtime Xbox 360.
6. Configurar runner Xenon, gerar `build/default.xex` e testar no console.

## Regras de progresso

Código importado não significa integração completa. Testes importados não significam aprovação no novo repositório. Build host, build Xenon, geração do XEX e execução no console são marcos separados.
