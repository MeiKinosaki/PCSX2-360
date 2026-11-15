# PCSX2-360 — bloqueios técnicos conhecidos

Este documento separa o que já existe no repositório do que ainda impede uma build funcional e uma execução real.

## Host / CI
- A regressão da biblioteca de jogos foi adicionada a `make check`.
- O teste cobre duplicidade de serial, filtros, favoritos, strings sem terminador e limite de 256 entradas.
- A aprovação CI desse conjunto só pode ser declarada depois de uma execução do workflow no GitHub Actions.

## Núcleo PS2
- O núcleo EE continua parcial; não representa uma CPU R5900 completa.
- Existe uma arbitragem experimental de interrupções usando Status.IE/IM, Cause.IP e EXL/ERL, com amostragem no limite entre instruções e deferimento em delay slot; ainda não há fontes de IRQ dos dispositivos, temporização completa, máscaras específicas EE nem auditoria diferencial R5900.
- A auditoria diferencial de exceções, PageMask, PFN, delay slots e semântica específica do R5900 continua aberta.
- Existe um scaffold inicial dos 10 canais DMA EE (CHCR/MADR/QWC/TADR) e teste de registradores; ainda não há motor de transferência, chain mode, arbitragem, integração com dispositivos, scheduler ou interrupções DMA.
- IOP, DMA, VU0/VU1, VIF, GIF, GS, SPU2 e CDVD ainda não formam uma máquina PS2 executável.
- O snapshot EE + TLB + RAM agora usa serialização campo a campo little-endian, versão 2 e checksum; o carregamento valida o TLB antes de alterar CPU/RAM. A implementação e os testes ainda aguardam execução confirmada no CI. O formato exige RAM de mesmo tamanho e ainda não inclui IOP/DMA/VU/GS nem persistência integrada ao runtime.

## Biblioteca e interface
- Existe apenas o modelo de dados da biblioteca; ainda não há scanner real de ISO/CSO/CHD nem leitura de USB/HDD.
- O modelo de biblioteca agora tem serialização versionada/checksum para favoritos e metadados, mas ainda não há integração com arquivo/dispositivo de armazenamento persistente no runtime.
- Os caminhos de capa são metadados; não há renderização de capas nem interface Aurora/Xbox 360.
- Download de capas depende de rede Xbox 360, HTTP/TLS, armazenamento e fonte de metadados; nada disso está integrado.
- A tela de abertura PS2 e o áudio não estão implementados. A música original do BIOS não deve ser redistribuída; a implementação deve depender de conteúdo fornecido legalmente pelo usuário ou de áudio original.

## Xbox 360 / XEX
- `src/main.c` ainda é um bootstrap de demonstração e não inicializa vídeo, áudio, controles, armazenamento ou rede.
- O alvo `make xex` requer `xenon-gcc`, bibliotecas libXenon e `elf2xex`.
- O workflow de XEX exige um runner self-hosted com labels `linux,xenon`; não há evidência de runner configurado nem de artefato `build/default.xex`.
- Um ELF intermediário não é o produto final; o requisito de entrega é `build/default.xex` validado.

## Ordem de resolução
1. Fazer `make check` passar continuamente no CI host.
2. Tornar o snapshot portátil e expandir cobertura de regressões EE.
3. Completar o caminho de execução do EE e depois implementar IOP/DMA/VU/GIF/GS/SPU2/CDVD.
4. Implementar scanner e persistência da biblioteca.
5. Criar runtime Xbox 360 (vídeo, áudio, input, armazenamento e rede).
6. Provisionar runner Xenon, gerar `build/default.xex`, inspecionar o artefato e testar em console RGH/JTAG.

Nenhum item dependente de CI, toolchain, runner ou console é considerado concluído sem evidência correspondente.
