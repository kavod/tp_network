/*
 * recoit.c
 * Attente d'un datagramme sur une socket UDP
 *
 * Travaux Pratiques r�seau SILR 1
 * Nicolas Normand
 * 1999-2001
 */

#include <unistd.h>

#include "primitives.h"

int recoit (int prise, char *buffer, size_t taille)
{
//	printf("Nous sommes rentr� dans la fonction recoit et nous sommes pr�t �... recevoir pardis !!!\n");
    if (read (prise, buffer, taille) >= 0)
    {
  //  	printf("Alors l�, nous allons sortir avec succ�s de la fonction recoit\n");
	return 0;
    }
    else
    {
	//printf("aie aie aie, la fonction recoit a plant���������");
	return -1;
    }
}

