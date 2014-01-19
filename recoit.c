/*
 * recoit.c
 * Attente d'un datagramme sur une socket UDP
 *
 * Travaux Pratiques réseau SILR 1
 * Nicolas Normand
 * 1999-2001
 */

#include <unistd.h>

#include "primitives.h"

int recoit (int prise, char *buffer, size_t taille)
{
//	printf("Nous sommes rentré dans la fonction recoit et nous sommes prêt à... recevoir pardis !!!\n");
    if (read (prise, buffer, taille) >= 0)
    {
  //  	printf("Alors là, nous allons sortir avec succès de la fonction recoit\n");
	return 0;
    }
    else
    {
	//printf("aie aie aie, la fonction recoit a plantééééééééé");
	return -1;
    }
}

