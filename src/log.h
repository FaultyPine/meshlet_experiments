#pragma once



void LogMessage(const char* message, ...);
#define LOG_INFO(message, ...) LogMessage(message __VA_OPT__(,) __VA_ARGS__)
