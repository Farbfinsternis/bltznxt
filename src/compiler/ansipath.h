#ifndef BLITZNEXT_ANSIPATH_H
#define BLITZNEXT_ANSIPATH_H

#include <filesystem>
#include <string>

// Pfade im Compiler sind Bytes in der ANSI-Codepage (unter deutschem Windows
// Windows-1252): so kommen sie aus argv, so stehen sie in einem Quelltext aus
// der Blitz-IDE ("Include ..."), so erwartet die IDE sie in Meldungen.
// std::filesystem::path liest einen std::string unter MinGW dagegen als UTF-8
// und wirft bei einem Umlaut - "blitzcc C:\Users\Juergen-mit-Umlaut\x.bb"
// endete mit filesystem_error (BUG-201). An jeder Grenze zu std::filesystem
// und zur Windows-API deshalb ueber diese Helfer wandeln.
//
// Definiert in blitzcc.cpp nach <windows.h>, damit dessen Makros nicht in die
// Projektheader geraten.
std::filesystem::path ansiPath(const std::string &ansi);    // ANSI-Bytes -> Pfad
std::string ansiString(const std::filesystem::path &p);     // Pfad -> ANSI-Bytes
std::string ansiGeneric(const std::filesystem::path &p);    // dasselbe mit '/'
std::wstring ansiWide(const std::string &ansi);             // ANSI-Bytes -> UTF-16

#endif // BLITZNEXT_ANSIPATH_H
