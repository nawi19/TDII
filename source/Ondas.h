/*
 Libreria de ondas ondas.h 
*/
#ifndef ONDAS_H_
#define ONDAS_H_
#include <stdint.h>
typedef struct {//Toda la misma forma, pero con mas o menos muestras
uint16_t *id; // puntero al arreglo de muestras
uint16_t cant; // cantidad de muestras
} SENAL;
typedef struct { // Struct de señales
 SENAL *senal;
 uint16_t pasos;
 char nombre[16]; // nombre de la señal
} LISTA;
//Declaración de las señales
extern uint16_t senoidal_6[6];
extern uint16_t senoidal_12[12];
extern uint16_t senoidal_24[24];
extern uint16_t senoidal_36[36];
extern uint16_t senoidal_48[48];
extern uint16_t triangular_6[6];
extern uint16_t triangular_12[12];
extern uint16_t triangular_24[24];
extern uint16_t triangular_36[36];
extern uint16_t triangular_48[48];
extern uint16_t cuadrada_2[2];
extern LISTA lista[]; //Lista de señales
extern SENAL senoidal[];
extern SENAL triangular[];
extern SENAL cuadrada[];
extern const uint32_t cant_formas; //cantidad de formas disponibles
#endif /* ONDAS_H_ */
