/*
 * envoie.c
 * �mission d'un datagramme sur une socket UDP "connect�e"
 *
 * Travaux Pratiques r�seau SILR 1
 * Nicolas Normand
 * 1999-2001
 */

#include <unistd.h>

#include "primitives.h"

int envoie (int prise, char *buffer, size_t taille)
{
//	printf("Nous sommes entr�s dans la fonction envoie\n");
    if (write (prise, buffer, taille) == taille)
    {
  //  	printf("Et nous en sortons\n");
	return 0;
	printf("Normalement, nous ne sommes pas sens� voir cette phrase...\n");
    }
    else
    {
	printf("aie aie aie, la fonction envoie a plant�\n");
	return -1;
    }
}

