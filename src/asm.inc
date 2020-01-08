%ifidn __OUTPUT_FORMAT__, macho32
%define JPR_BITS 32
%define FUNC(x) _ %+ x
%elifidn __OUTPUT_FORMAT__, elf32
%define JPR_BITS 32
%define FUNC(x) x
%elifidn __OUTPUT_FORMAT__, win32
%define JPR_BITS 32
%define FUNC(x) _ %+ x
%elifidn __OUTPUT_FORMAT__, macho64
%define JPR_BITS 64
%define FUNC(x) _ %+ x
%elifidn __OUTPUT_FORMAT__, elf64
%define JPR_BITS 64
%define FUNC(x) x
%elifidn __OUTPUT_FORMAT__, win64
%define JPR_BITS 64
%define FUNC(x) x
%endif

BITS JPR_BITS
