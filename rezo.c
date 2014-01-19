      /****************************/
     /* Projet Réseau            */
    /* Protocole 1              */
   /* Auteurs : Brice GRICHY   */
  /*           Loïc Jouhannet */
 /* Date : 23/01/2003        */
/****************************/

/* commentaires :
 *		-ce qui va bien :) compatible avec le logiciel de HAY Céline - Retailleau Agnès et Colineaux François - Gicquel Sébastien.
 *		-ce qui ne va pas :( Comme nous utilisons les noms (ou adresse) des postes pour identifier expéditeurs/destinataires,
 *			le fonctionnement du logiciel à partir de shells d'un même poste peut impliquer certaines bizarreries :
 *			   # déconnexion sans réorganisation du réseau (en effet, tout les postes vont se sentir concernés par la procédure de déconnexion)
 *			   # La diffusion ne se fait pas : le shell suivant se reconnait expéditeur et va donc détruire la trame.
 *			   # La transmission de message ne fonctionne pas avec 3 shell sur un même poste : chaque shell se reconnait destinataire.
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
#include "creePriseEmission.c"			// Fonction de création de prise d'émission
#include "creePriseReception.c"			// Fonction de création de prise Réception
#include "envoie.c"				// Fonction d'envoi de données 
#include "recoit.c"				// Fonction de réceptions de données

typedef struct					// Structure de la trame du protocole
{
	char dest [16];				// Adresse du destinataire de la trame
	char exp [16];				// Adresse de l'expéditeur de la trame
	char typeTrame;				// Type de la trame : '0' => déconnexion, '1' => Accusé de réception, '2' => Rejet de la trame, '3' => Trame d'information
	int taille;				// Taille du champs donnee (sauf pour déconnexion : contient le port d'émission de l'entité déconnectante)
	char donnee [LONGUEUR_MAX];		// Données de la trame d'information (pour trame de déconnexion : nom de l'entité suivante de l'entité déconnectante sur l'anneau)
} trame;
typedef trame *trame_ptr;

	/* variables d'environnnement*/
	char * adr_svt;				// Adresse de l'entité à qui on émet
	char * monAdresse;			// Adresse de notre propre poste
	char * adr_dest; 			// Adresse du destinataire de notre trame
	int portEmission, portReception;	//Ports de communications
	int priseEmission, priseReception;	//prises de communication

	int jetonLibre(void);			// Fonction exécutée après réception d'un jeton libre
	int creaTrame(int, char *, char *);	// Fonction de création et d'envoi de trames
	int fctTraiter(trame_ptr);		// Fonction exécutée à la suite d'une trame

int main()
{
	char rep;	                    	// Variable de réponse pour être l'émetteur ou non
	int deco;				// Valeur test de fin de programme. Reste à 0 tant que l'utilisateur ne souhaite pas se déconnecter
	char * buffer;				// Buffer réceptionnant les trames reçues
	trame_ptr maTrame;			// Trames reçues formatée sous forme de structure trame_ptr

	printf("version 1.3\n");
	
	  /**************************/
	 /*initialisation du réseau*/
	/**************************/
	
	monAdresse = (char *)calloc(15,sizeof(char)); //| Allocation dynamique de l'espace mémoire
	adr_svt = (char *)calloc(4,sizeof(char));     //|
	maTrame = malloc(sizeof(trame));	      //|
	buffer = (char *)calloc(LONGUEUR_MAX+16+16+1+4,sizeof(char));

	printf("Veuillez entrer le nom de votre machine :\n");		// Attente du nom (ou de l'adresse IP) de la machine sur laquelle on travaille
	scanf("%s",monAdresse);						// Réception dans monAdresse (chaine de caractères). Il serait intértessant d'interdire les adresses de plus de 15 caractères
	printf("\nVoulez-vous émettre le premier jeton ? (Y/N)\n");	// Voulez-vous émettre le jeton libre qui initialisera le réseau
	rep = '\000';							// initialisation de la variable réponse pour éviter toute surprise avec un emplacement mémoire déjà plein
	while (rep != 89 && rep !=121 && rep!=110 && rep!=78)		// Tant que les réponses ne sont pas intelligibles (Y, y, N ou n)
	{
		rep = getchar();					// réception de la réponse de l'utilisateur
	}
	printf("\nEntrez l'adresse d'émission\n");			// Saisie de l'adresse du poste auquel nous allons émettre
	scanf("%s",adr_svt);
	printf("Entrez maintenant le port d'émission\n");		// Saisie du port d'émission
	scanf("%d",&portEmission);
	printf("Entrez maintenant le port de réception\n");		// et du port de réception
	scanf("%d",&portReception);
	priseEmission = creePriseEmission (adr_svt,portEmission);	// Création des prises
	priseReception = creePriseReception (portReception);
	if ((rep == 'Y') || (rep == 'y'))				// Si nous somme premier émetteur
	{
		printf("Lorsque tous les postes seront prêts, entrez un caractère au choix puis ENTER\n"); // On attend la connexion des autres postes
		scanf("%d",&deco);					// On attend que l'utilisateur ait donné l'ordre de lancer le jeton
		if (envoie(priseEmission,JETON,16)==-1)			// On envoie le jeton
		{
			printf("Erreur de l'initialisation\n");		// Si il y a eu un problème d'envoi, le signaler
			return 0;
		}
	}

	  /****************************/
	 /* Ecoute du port réception */
	/****************************/

	deco = 0;							// Initialisation de deco, elle restera à 0 tant que l'utilisateur ne souhaitera pas se déconnecter
	while (deco == 0)
	{//
		if (recoit(priseReception,buffer,LONGUEUR_MAX+16+16+1+4)==-1) // Ecoute du port Réception. On réceptionne par défaut la plus grande trame possible (LONGUEUR_TRAME + la longueur d'une en-tête
		{
			printf("Erreur de transmission\n");		// En cas d'erreur, le signaler
			return 0;
		}
		else
		{
			maTrame = (trame_ptr)buffer;			// Conversion de la chaine de caractères recue en un objet de type trame

			/*printf("\nDest : %s\n",maTrame->dest);		// Affichage de la trame reçue
			printf("Exp : %s\n",maTrame->exp);		// facilitant de débugage
			printf("Type : %c\n",maTrame->typeTrame);	// en cas de besoin
			printf("Longueur : %d\n",maTrame->taille);
			printf("Donnes : %s\n",maTrame->donnee);     */


			if (strcmp(buffer,JETON)==0)			// Si la trame recu est un jeton
			{
				deco = jetonLibre();			// proposer à l'utilisateur d'envoyer un message ou d'exécuter une commande
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
 /* Dans le cad d'une réception d'un jeton libre */
/************************************************/

int jetonLibre()							// Renvoie 0 si tout se passe bien, -1 si une erreur est rencontrée afin de déconnecter de façon critique le poste
{
	char rep;							// variable de choix de l'utilisateur
	char * buffer;							// variable de stockage du message à envoyer
	
	buffer = (char *) calloc(LONGUEUR_MAX+16+16+1+4,sizeof(char));  //| Allocation dynamique de la mémoire
	adr_dest = (char *)calloc(4,sizeof(char));  			//|
	
	printf("\nVoulez-vous envoyer un message ? [Y/N]\n");		// |On demande à l'utilisateur s'il veut prendre la main
	while (rep != 89 && rep !=121 && rep!=110 && rep!=78)		// |Tant que la réponse n'est pas intelligible (Y,y, N ou n)
	{								// |
		rep = getchar();					// |On saisit le choix de l'utilisateur
	}
	if ((rep == 'Y') || (rep == 'y'))				// Si c'est le cas, on demande le destinataire du futur message.
	{								// La commande "quit" permet d'entamer la procédure de déconnexion
		printf("A qui voulez-vous envoyer ce message ? (\"quit\" pour quitter, \"cancel\" pour annuler)\n"); // la commande "cancel" permet de revenir sur son choix de prendre la main
		scanf("%s",adr_dest);					// Saisie de l'adresse destinataire (ou de la commande quit/cancel)
		if (strcmp(adr_dest,"quit")==0)				// Si l'ulitilisateur a rentré quit => procédure de déconnexion
		{
			printf("Procédure de déconnexion, bye bye\n");
			if (!creaTrame(0,"rien","rien"))		// Création et envoie d'une trame de déconnexion (codée 0), les deux autres arguments sont inutiles dans ce cas là
			{
				printf("Erreur\n");			// En cas d'erreur, le signaler
				return -1;				// et renvoyer -1 (code de deco pour la déconnexion critique)
			}
			return 1;					// Sinon, renvoyr 1 (code de deco pour la deconnexion propre)
		} else
		{
			if (strcmp(adr_dest,"cancel")!=0)		// Si la commande n'est pas "cancel"
			{
				printf("Entrez le message\n");		// Saisie du message de l'utilisateur
				gets(buffer);
				gets(buffer);
				creaTrame(3,adr_dest,buffer);		// Création et envoie de la trame d'information à adr_dest
				return 0;				// Renvoie 0 (code de deco pour ne pas déconnecter)
			} else
			{
				if (envoie(priseEmission,JETON,16)==-1) // Si la commande est "cancel", envoyer un jeton libre
				{
					printf("Erreur de transmission\n"); // prévenir en cas d'erreur
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
 /* Création et envoi des trames */
/********************************/
// 1er paramètre : type de la trame (0 pour déconnexion, 1 pour Accusée de Réception, 2 pour Rejet de la trame, 3 pour trame d'information
// 2nd paramètre : adresse du poste destinataire (DIFFUSION en cas de message de diffusion)
// 3ème paramètre : données à envoyer
int creaTrame(int typeTram, char * destinataire, char * donnees)	// Renvoie 1 en cas de bon fonctionnement, 0 sinon
{
	trame_ptr maTrame;					// Déclaration d'une nouvelle trame

	maTrame=malloc(sizeof(trame)*1);			// Allocation dynamique de la mémoire
	switch (typeTram)					// Aiguillage en fonction du type de trame
	{
		case 3:
		{
			// Trame d'information
			strcpy(maTrame->dest,destinataire);	// Destinataire entré en paramètre
			strcpy(maTrame->exp,monAdresse);	// Expéditeur : soi-même
			maTrame -> typeTrame = '3';		// Type de trame info codée '3'
			maTrame->taille = strlen(donnees);	// Taille des données
			strcpy(maTrame->donnee,donnees);	// Données entrées en paramètre
			envoie(priseEmission,(char *) maTrame,TAILLE_TRAME); // Envoie de la trame, convertie en chaine de caractères
			
			//affichage de la trame envoyée facilitant le débugage
			/*printf("J'envoie %s\n",((trame_ptr)((char *)maTrame))->exp);
			printf("J'envoie %s\n",((trame_ptr)((char *)maTrame))->dest);
			printf("J'envoie %c\n",((trame_ptr)((char *)maTrame))->typeTrame);
			printf("J'envoie %d\n",((trame_ptr)((char *)maTrame))->taille);
			printf("J'envoie %s\n",((trame_ptr)((char *)maTrame))->donnee); */

			return 1;
		}
		case 1:
		{
			// Accusé de Réception
			strcpy(maTrame->dest,destinataire);	// Destinataire entré en paramètre
			strcpy(maTrame->exp,monAdresse);	// Expéditeur : soi-même
			maTrame->typeTrame = '1';		// Type de trame codé '1'
			maTrame->taille = 0;			//| Pas de données, donc taille nulle
			strcpy(maTrame->donnee,"\0");		//|
			envoie(priseEmission,(char *)(maTrame),TAILLE_TRAME); // envoi
			return 1;
		}
		case 2:
		{
			// Rejet
			strcpy(maTrame->dest,destinataire);	//| Idem
			strcpy(maTrame->exp,monAdresse);	//|
			maTrame->typeTrame = '2';		//| (type de la trame codée '2')
			maTrame->taille = 0;			//|
			strcpy(maTrame->donnee,"\0");		//|
			envoie(priseEmission,(char *)(maTrame),TAILLE_TRAME);
			return 1;
		}
		case 0:
		{
			// Trame de deconnexion
			strcpy(maTrame->dest,DIFFUSION);	// Message de diffusion
			strcpy(maTrame->exp,monAdresse);	// Expéditeur : soi-même
			maTrame->typeTrame = '0';		// type de trame codée 0
			maTrame->taille = portEmission;		// la taille est remplacée par le port d'émission à transmettre
			strcpy(maTrame->donnee,adr_svt);	// Les données sont l'adresse du poste à qui on émettais avant déconnexion afin d'assurer la continuité de l'anneau après notre propre départ
			envoie(priseEmission,(char *)(maTrame),TAILLE_TRAME);
			
			//Affichage de la trame envoyée afin de facilité de débugage
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
			printf("Le type n'est pas bon.\n");	// Type inconnu (différent de 0, 1, 2, 3)
			return 0;
		}
	}
	return 0;
}

  /**************************/
 /* traitement de la trame */
/**************************/
int fctTraiter(trame_ptr maTrame)   // Fonction qui prend une trame en parametre et qui renvoie un entier (0 si on ne se déconnecte pas, 1 si on entame une procédure de déconnexion, -1 si on effectue une déconnexion critique (finalisation du programme))
{
	if (strcmp(maTrame->dest,DIFFUSION)==0)        // Si on reconnait une trame de diffusion
	{
		if (maTrame->typeTrame == '0')         // Si c'est une trame de déconnexion (type codé 0)
		{
			if (strcmp(adr_svt,maTrame->exp) == 0)	                        // Si c'est le poste suivant sur l'anneau qui se déconnecte...
			{
				if (maTrame->taille == portReception)                   // ... et que notre poste se retrouve seul sur l'anneau (c'est à dire que la prise d'émission envoie sur notre prise de réception)...
				{
					printf("Il n'y a plus de reseau. Bye Bye.\n");  // ... on termine le programme.
					return -1;
				}
				else
				{
					printf("Réorganisation des prises : %s s'est déconnecté\n",maTrame->exp);  // Si il reste au moins 2 postes sur l'anneau...
					priseEmission = creePriseEmission(maTrame->donnee,maTrame->taille);        // ... on réorganise les prises...
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
				envoie(priseEmission, (char *)maTrame,TAILLE_TRAME );                              // Si la procédure de déconnexion ne nous concerne pas, on fait suivre.
				return 0;
			}
		}
		else
		{
			if (strcmp(maTrame->exp, monAdresse) == 0)                                                 // Si le message de diffusion est celui que j'ai envoyé...
			{
				if (envoie(priseEmission,JETON,16)==-1)                                            // ... on détruit la trame et on la remplace par un jeton libre
				{
					printf("Erreur de transmission\n");                                        // En cas d'erreur, le signaler
					return -1;
				}
				return 0;
			}
			else
			{
				printf("Ce msg est destiné à tout le monde > %s\n",maTrame->donnee);               // Si ce message n'est pas généré par moi, l'afficher
				envoie(priseEmission, (char *)maTrame,TAILLE_TRAME );                              // Et je fais suivre le message
				return 0;
			}
		}
	}
	else
	{
		if ( strcmp( maTrame->dest , monAdresse ) == 0)                                                    // Si ce n'est pas un message de diffusion et que la trame m'est destinée
		{
			if (maTrame->typeTrame == '3')                                                             // Est-ce qu'il s'agit d'une trame d'information
			{
				if (maTrame->taille == strlen(maTrame->donnee))                                    // Si c'est le cas : on teste l'intégrité avec le champ taille de la trame
				{
					printf("%s > %s\n",maTrame->exp,maTrame->donnee);			   // Si le message est correct, on l'affiche
					creaTrame(1,maTrame->exp,"rien");                                          // Et on envoie un accusé de réception
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
				if (maTrame->typeTrame == '1')                                                    // Si c'est un accusé de réception
				{
					printf("Votre message a bien été reçu\n");                                // on affiche un message de confirmation de confirmation
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
						printf("erreur de l'envoie du message");                          // le signaler à l'utilisateur
					}
					else
					{
						printf("Erreur de Trame\n");                                      // Si le type de la trame n'est pas connu (différent de 0,1,2,3) on affiche un message d'erreur
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
			if (strcmp(monAdresse,maTrame->exp) == 0)                                                 // Si ce message a été généré par moi...
			{
				printf("destinataire non trouvé\n");                                              // ... ce message a donc fait le tour de l'anneau sans trouver de destinataire, on affiche donc une erreur
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
