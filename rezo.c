      /****************************/
     /* Projet R�seau            */
    /* Protocole 1              */
   /* Auteurs : Brice GRICHY   */
  /*           Lo�c Jouhannet */
 /* Date : 23/01/2003        */
/****************************/

/* commentaires :
 *		-ce qui va bien :) compatible avec le logiciel de HAY C�line - Retailleau Agn�s et Colineaux Fran�ois - Gicquel S�bastien.
 *		-ce qui ne va pas :( Comme nous utilisons les noms (ou adresse) des postes pour identifier exp�diteurs/destinataires,
 *			le fonctionnement du logiciel � partir de shells d'un m�me poste peut impliquer certaines bizarreries :
 *			   # d�connexion sans r�organisation du r�seau (en effet, tout les postes vont se sentir concern�s par la proc�dure de d�connexion)
 *			   # La diffusion ne se fait pas : le shell suivant se reconnait exp�diteur et va donc d�truire la trame.
 *			   # La transmission de message ne fonctionne pas avec 3 shell sur un m�me poste : chaque shell se reconnait destinataire.
 */

#define LONGUEUR_MAX 800
#define JETON "111111111111111\0"                             				
#define TAILLE_TRAME sizeof(maTrame->exp)+sizeof(maTrame->dest)+sizeof(maTrame->taille)+sizeof(maTrame->typeTrame)+maTrame->taille+4
#define DIFFUSION "diffusion"

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include <string.h>

#include "primitives.h"				// Prototypes des fonctions de communication
#include "creePriseEmission.c"			// Fonction de cr�ation de prise d'�mission
#include "creePriseReception.c"			// Fonction de cr�ation de prise R�ception
#include "envoie.c"				// Fonction d'envoi de donn�es 
#include "recoit.c"				// Fonction de r�ceptions de donn�es

typedef struct					// Structure de la trame du protocole
{
	char dest [16];				// Adresse du destinataire de la trame
	char exp [16];				// Adresse de l'exp�diteur de la trame
	char typeTrame;				// Type de la trame : '0' => d�connexion, '1' => Accus� de r�ception, '2' => Rejet de la trame, '3' => Trame d'information
	int taille;				// Taille du champs donnee (sauf pour d�connexion : contient le port d'�mission de l'entit� d�connectante)
	char donnee [LONGUEUR_MAX];		// Donn�es de la trame d'information (pour trame de d�connexion : nom de l'entit� suivante de l'entit� d�connectante sur l'anneau)
} trame;
typedef trame *trame_ptr;

	/* variables d'environnnement*/
	char * adr_svt;				// Adresse de l'entit� � qui on �met
	char * monAdresse;			// Adresse de notre propre poste
	char * adr_dest; 			// Adresse du destinataire de notre trame
	int portEmission, portReception;	//Ports de communications
	int priseEmission, priseReception;	//prises de communication

	int jetonLibre(void);			// Fonction ex�cut�e apr�s r�ception d'un jeton libre
	int creaTrame(int, char *, char *);	// Fonction de cr�ation et d'envoi de trames
	int fctTraiter(trame_ptr);		// Fonction ex�cut�e � la suite d'une trame

int main()
{
	char rep;	                    	// Variable de r�ponse pour �tre l'�metteur ou non
	int deco;				// Valeur test de fin de programme. Reste � 0 tant que l'utilisateur ne souhaite pas se d�connecter
	char * buffer;				// Buffer r�ceptionnant les trames re�ues
	trame_ptr maTrame;			// Trames re�ues format�e sous forme de structure trame_ptr

	printf("version 1.3\n");
	
	  /**************************/
	 /*initialisation du r�seau*/
	/**************************/
	
	monAdresse = (char *)calloc(15,sizeof(char)); //| Allocation dynamique de l'espace m�moire
	adr_svt = (char *)calloc(4,sizeof(char));     //|
	maTrame = malloc(sizeof(trame));	      //|
	buffer = (char *)calloc(LONGUEUR_MAX+16+16+1+4,sizeof(char));

	printf("Veuillez entrer le nom de votre machine :\n");		// Attente du nom (ou de l'adresse IP) de la machine sur laquelle on travaille
	scanf("%s",monAdresse);						// R�ception dans monAdresse (chaine de caract�res). Il serait int�rtessant d'interdire les adresses de plus de 15 caract�res
	printf("\nVoulez-vous �mettre le premier jeton ? (Y/N)\n");	// Voulez-vous �mettre le jeton libre qui initialisera le r�seau
	rep = '\000';							// initialisation de la variable r�ponse pour �viter toute surprise avec un emplacement m�moire d�j� plein
	while (rep != 89 && rep !=121 && rep!=110 && rep!=78)		// Tant que les r�ponses ne sont pas intelligibles (Y, y, N ou n)
	{
		rep = getchar();					// r�ception de la r�ponse de l'utilisateur
	}
	printf("\nEntrez l'adresse d'�mission\n");			// Saisie de l'adresse du poste auquel nous allons �mettre
	scanf("%s",adr_svt);
	printf("Entrez maintenant le port d'�mission\n");		// Saisie du port d'�mission
	scanf("%d",&portEmission);
	printf("Entrez maintenant le port de r�ception\n");		// et du port de r�ception
	scanf("%d",&portReception);
	priseEmission = creePriseEmission (adr_svt,portEmission);	// Cr�ation des prises
	priseReception = creePriseReception (portReception);
	if ((rep == 'Y') || (rep == 'y'))				// Si nous somme premier �metteur
	{
		printf("Lorsque tous les postes seront pr�ts, entrez un caract�re au choix puis ENTER\n"); // On attend la connexion des autres postes
		scanf("%d",&deco);					// On attend que l'utilisateur ait donn� l'ordre de lancer le jeton
		if (envoie(priseEmission,JETON,16)==-1)			// On envoie le jeton
		{
			printf("Erreur de l'initialisation\n");		// Si il y a eu un probl�me d'envoi, le signaler
			return 0;
		}
	}

	  /****************************/
	 /* Ecoute du port r�ception */
	/****************************/

	deco = 0;							// Initialisation de deco, elle restera � 0 tant que l'utilisateur ne souhaitera pas se d�connecter
	while (deco == 0)
	{//
		if (recoit(priseReception,buffer,LONGUEUR_MAX+16+16+1+4)==-1) // Ecoute du port R�ception. On r�ceptionne par d�faut la plus grande trame possible (LONGUEUR_TRAME + la longueur d'une en-t�te
		{
			printf("Erreur de transmission\n");		// En cas d'erreur, le signaler
			return 0;
		}
		else
		{
			maTrame = (trame_ptr)buffer;			// Conversion de la chaine de caract�res recue en un objet de type trame

			/*printf("\nDest : %s\n",maTrame->dest);		// Affichage de la trame re�ue
			printf("Exp : %s\n",maTrame->exp);		// facilitant de d�bugage
			printf("Type : %c\n",maTrame->typeTrame);	// en cas de besoin
			printf("Longueur : %d\n",maTrame->taille);
			printf("Donnes : %s\n",maTrame->donnee);     */


			if (strcmp(buffer,JETON)==0)			// Si la trame recu est un jeton
			{
				deco = jetonLibre();			// proposer � l'utilisateur d'envoyer un message ou d'ex�cuter une commande
			}
			else
			{
				deco = fctTraiter(maTrame);		// Sinon, traiter en fonction de la trame
			}
		}
	}
	return 1;
}

  /************************************************/
 /* Dans le cad d'une r�ception d'un jeton libre */
/************************************************/

int jetonLibre()							// Renvoie 0 si tout se passe bien, -1 si une erreur est rencontr�e afin de d�connecter de fa�on critique le poste
{
	char rep;							// variable de choix de l'utilisateur
	char * buffer;							// variable de stockage du message � envoyer
	
	buffer = (char *) calloc(LONGUEUR_MAX+16+16+1+4,sizeof(char));  //| Allocation dynamique de la m�moire
	adr_dest = (char *)calloc(4,sizeof(char));  			//|
	
	printf("\nVoulez-vous envoyer un message ? [Y/N]\n");		// |On demande � l'utilisateur s'il veut prendre la main
	while (rep != 89 && rep !=121 && rep!=110 && rep!=78)		// |Tant que la r�ponse n'est pas intelligible (Y,y, N ou n)
	{								// |
		rep = getchar();					// |On saisit le choix de l'utilisateur
	}
	if ((rep == 'Y') || (rep == 'y'))				// Si c'est le cas, on demande le destinataire du futur message.
	{								// La commande "quit" permet d'entamer la proc�dure de d�connexion
		printf("A qui voulez-vous envoyer ce message ? (\"quit\" pour quitter, \"cancel\" pour annuler)\n"); // la commande "cancel" permet de revenir sur son choix de prendre la main
		scanf("%s",adr_dest);					// Saisie de l'adresse destinataire (ou de la commande quit/cancel)
		if (strcmp(adr_dest,"quit")==0)				// Si l'ulitilisateur a rentr� quit => proc�dure de d�connexion
		{
			printf("Proc�dure de d�connexion, bye bye\n");
			if (!creaTrame(0,"rien","rien"))		// Cr�ation et envoie d'une trame de d�connexion (cod�e 0), les deux autres arguments sont inutiles dans ce cas l�
			{
				printf("Erreur\n");			// En cas d'erreur, le signaler
				return -1;				// et renvoyer -1 (code de deco pour la d�connexion critique)
			}
			return 1;					// Sinon, renvoyr 1 (code de deco pour la deconnexion propre)
		} else
		{
			if (strcmp(adr_dest,"cancel")!=0)		// Si la commande n'est pas "cancel"
			{
				printf("Entrez le message\n");		// Saisie du message de l'utilisateur
				gets(buffer);
				gets(buffer);
				creaTrame(3,adr_dest,buffer);		// Cr�ation et envoie de la trame d'information � adr_dest
				return 0;				// Renvoie 0 (code de deco pour ne pas d�connecter)
			} else
			{
				if (envoie(priseEmission,JETON,16)==-1) // Si la commande est "cancel", envoyer un jeton libre
				{
					printf("Erreur de transmission\n"); // pr�venir en cas d'erreur
					return -1;
				}
				return 0;
			}
		}
		return -1;
	} else
	{
		if (envoie(priseEmission,JETON,16)==-1)			// Si l'utilisateur ne souhaite pas envoyer de message, envoi d'un jeton libre
		{
			printf("Erreur de transmission\n");		// En cas d'erreur, le signaler
			return -1;
		}
	}
	return 0;
}
  /********************************/
 /* Cr�ation et envoi des trames */
/********************************/
// 1er param�tre : type de la trame (0 pour d�connexion, 1 pour Accus�e de R�ception, 2 pour Rejet de la trame, 3 pour trame d'information
// 2nd param�tre : adresse du poste destinataire (DIFFUSION en cas de message de diffusion)
// 3�me param�tre : donn�es � envoyer
int creaTrame(int typeTram, char * destinataire, char * donnees)	// Renvoie 1 en cas de bon fonctionnement, 0 sinon
{
	trame_ptr maTrame;					// D�claration d'une nouvelle trame

	maTrame=malloc(sizeof(trame)*1);			// Allocation dynamique de la m�moire
	switch (typeTram)					// Aiguillage en fonction du type de trame
	{
		case 3:
		{
			// Trame d'information
			strcpy(maTrame->dest,destinataire);	// Destinataire entr� en param�tre
			strcpy(maTrame->exp,monAdresse);	// Exp�diteur : soi-m�me
			maTrame -> typeTrame = '3';		// Type de trame info cod�e '3'
			maTrame->taille = strlen(donnees);	// Taille des donn�es
			strcpy(maTrame->donnee,donnees);	// Donn�es entr�es en param�tre
			envoie(priseEmission,(char *) maTrame,TAILLE_TRAME); // Envoie de la trame, convertie en chaine de caract�res
			
			//affichage de la trame envoy�e facilitant le d�bugage
			/*printf("J'envoie %s\n",((trame_ptr)((char *)maTrame))->exp);
			printf("J'envoie %s\n",((trame_ptr)((char *)maTrame))->dest);
			printf("J'envoie %c\n",((trame_ptr)((char *)maTrame))->typeTrame);
			printf("J'envoie %d\n",((trame_ptr)((char *)maTrame))->taille);
			printf("J'envoie %s\n",((trame_ptr)((char *)maTrame))->donnee); */

			return 1;
		}
		case 1:
		{
			// Accus� de R�ception
			strcpy(maTrame->dest,destinataire);	// Destinataire entr� en param�tre
			strcpy(maTrame->exp,monAdresse);	// Exp�diteur : soi-m�me
			maTrame->typeTrame = '1';		// Type de trame cod� '1'
			maTrame->taille = 0;			//| Pas de donn�es, donc taille nulle
			strcpy(maTrame->donnee,"\0");		//|
			envoie(priseEmission,(char *)(maTrame),TAILLE_TRAME); // envoi
			return 1;
		}
		case 2:
		{
			// Rejet
			strcpy(maTrame->dest,destinataire);	//| Idem
			strcpy(maTrame->exp,monAdresse);	//|
			maTrame->typeTrame = '2';		//| (type de la trame cod�e '2')
			maTrame->taille = 0;			//|
			strcpy(maTrame->donnee,"\0");		//|
			envoie(priseEmission,(char *)(maTrame),TAILLE_TRAME);
			return 1;
		}
		case 0:
		{
			// Trame de deconnexion
			strcpy(maTrame->dest,DIFFUSION);	// Message de diffusion
			strcpy(maTrame->exp,monAdresse);	// Exp�diteur : soi-m�me
			maTrame->typeTrame = '0';		// type de trame cod�e 0
			maTrame->taille = portEmission;		// la taille est remplac�e par le port d'�mission � transmettre
			strcpy(maTrame->donnee,adr_svt);	// Les donn�es sont l'adresse du poste � qui on �mettais avant d�connexion afin d'assurer la continuit� de l'anneau apr�s notre propre d�part
			envoie(priseEmission,(char *)(maTrame),TAILLE_TRAME);
			
			//Affichage de la trame envoy�e afin de facilit� de d�bugage
			/*printf("J'envoie %s d'une taille de %d\n",((trame_ptr)((char *)maTrame))->exp,sizeof(((trame_ptr)((char *)maTrame))->exp));
			printf("J'envoie %s d'une taille de %d\n",((trame_ptr)((char *)maTrame))->dest,sizeof(((trame_ptr)((char *)maTrame))->dest));
			printf("J'envoie %c d'une taille de %d\n",((trame_ptr)((char *)maTrame))->typeTrame,sizeof(((trame_ptr)((char *)maTrame))->typeTrame));
			printf("J'envoie %d d'une taille de %d\n",((trame_ptr)((char *)maTrame))->taille,sizeof(((trame_ptr)((char *)maTrame))->taille));
			printf("J'envoie %s d'une taille de %d\n",((trame_ptr)((char *)maTrame))->donnee,sizeof(((trame_ptr)((char *)maTrame))->donnee)); */

			return 1;
		}
		default:
		{
			// Erreur
			printf("Le type n'est pas bon.\n");	// Type inconnu (diff�rent de 0, 1, 2, 3)
			return 0;
		}
	}
	return 0;
}

  /**************************/
 /* traitement de la trame */
/**************************/
int fctTraiter(trame_ptr maTrame)   // Fonction qui prend une trame en parametre et qui renvoie un entier (0 si on ne se d�connecte pas, 1 si on entame une proc�dure de d�connexion, -1 si on effectue une d�connexion critique (finalisation du programme))
{
	if (strcmp(maTrame->dest,DIFFUSION)==0)        // Si on reconnait une trame de diffusion
	{
		if (maTrame->typeTrame == '0')         // Si c'est une trame de d�connexion (type cod� 0)
		{
			if (strcmp(adr_svt,maTrame->exp) == 0)	                        // Si c'est le poste suivant sur l'anneau qui se d�connecte...
			{
				if (maTrame->taille == portReception)                   // ... et que notre poste se retrouve seul sur l'anneau (c'est � dire que la prise d'�mission envoie sur notre prise de r�ception)...
				{
					printf("Il n'y a plus de reseau. Bye Bye.\n");  // ... on termine le programme.
					return -1;
				}
				else
				{
					printf("R�organisation des prises : %s s'est d�connect�\n",maTrame->exp);  // Si il reste au moins 2 postes sur l'anneau...
					priseEmission = creePriseEmission(maTrame->donnee,maTrame->taille);        // ... on r�organise les prises...
					if (envoie(priseEmission,JETON,16)==-1)                                    // ... et on envoie un jeton libre.
					{
						printf("Erreur de transmission\n");                                // En cas d'erreur d'envoi du jeton le signaler
						return -1;
					}
				}
				return 0;
			}
			else
			{
				//faire suivre
				envoie(priseEmission, (char *)maTrame,TAILLE_TRAME );                              // Si la proc�dure de d�connexion ne nous concerne pas, on fait suivre.
				return 0;
			}
		}
		else
		{
			if (strcmp(maTrame->exp, monAdresse) == 0)                                                 // Si le message de diffusion est celui que j'ai envoy�...
			{
				if (envoie(priseEmission,JETON,16)==-1)                                            // ... on d�truit la trame et on la remplace par un jeton libre
				{
					printf("Erreur de transmission\n");                                        // En cas d'erreur, le signaler
					return -1;
				}
				return 0;
			}
			else
			{
				printf("Ce msg est destin� � tout le monde > %s\n",maTrame->donnee);               // Si ce message n'est pas g�n�r� par moi, l'afficher
				envoie(priseEmission, (char *)maTrame,TAILLE_TRAME );                              // Et je fais suivre le message
				return 0;
			}
		}
	}
	else
	{
		if ( strcmp( maTrame->dest , monAdresse ) == 0)                                                    // Si ce n'est pas un message de diffusion et que la trame m'est destin�e
		{
			if (maTrame->typeTrame == '3')                                                             // Est-ce qu'il s'agit d'une trame d'information
			{
				if (maTrame->taille == strlen(maTrame->donnee))                                    // Si c'est le cas : on teste l'int�grit� avec le champ taille de la trame
				{
					printf("%s > %s\n",maTrame->exp,maTrame->donnee);			   // Si le message est correct, on l'affiche
					creaTrame(1,maTrame->exp,"rien");                                          // Et on envoie un accus� de r�ception
					return 0;
				}
				else
				{
					creaTrame(2,maTrame->exp,"rien");                                          // Si le message semble incorrect, on envoie une trame de rejet
					return 0;
				}
			}
			else
			{
				if (maTrame->typeTrame == '1')                                                    // Si c'est un accus� de r�ception
				{
					printf("Votre message a bien �t� re�u\n");                                // on affiche un message de confirmation de confirmation
					if (envoie(priseEmission,JETON,16)==-1)                                   // et on envoie un jeton libre
					{
						printf("Erreur de transmission\n");                               // En cas d'erreur d'envoi, le signaler
						return -1;
					}
					return 0;
				}
				else
				{
					if (maTrame->typeTrame == '2')                                            // En cas de rejet de notre message
					{
						printf("erreur de l'envoie du message");                          // le signaler � l'utilisateur
					}
					else
					{
						printf("Erreur de Trame\n");                                      // Si le type de la trame n'est pas connu (diff�rent de 0,1,2,3) on affiche un message d'erreur
						if (envoie(priseEmission, (char *)maTrame,TAILLE_TRAME ))         // et on fait suivre la trame
						{
							printf("Erreur de transmission\n");                       // En cas d'erreur d'envoi, le signaler
							return -1;
						}
						return 0;
					}
				}
			}
		}
		else
		{
			if (strcmp(monAdresse,maTrame->exp) == 0)                                                 // Si ce message a �t� g�n�r� par moi...
			{
				printf("destinataire non trouv�\n");                                              // ... ce message a donc fait le tour de l'anneau sans trouver de destinataire, on affiche donc une erreur
				if (envoie(priseEmission,JETON,16)==-1)                                           // On envoie un jeton
				{
					printf("Erreur de transmission\n");                                       // En cas d'erreur, le signaler
					return -1;
				}
				return 0;
			}
			else
			{
				envoie(priseEmission, (char *)maTrame,TAILLE_TRAME );                             // Si ce n'est pas pour moi, que ce n'est pas de moi, que ce n'est pas une trame de diffusion, on fait suivre la trame
				return 0;
			}
		}
	}
	return -1;
}
