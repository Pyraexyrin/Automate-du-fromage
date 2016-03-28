/*
 *   Ce fichier fait partie d'un projet de programmation donné en Licence 3 
 *   à l'Université de Bordeaux
 *
 *   Copyright (C) 2014, 2015 Adrien Boussicault
 *
 *    This Library is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This Library is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this Library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "automate.h"
#include "table.h"
#include "ensemble.h"
#include "outils.h"
#include "fifo.h"

#include <search.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h> 

#include <assert.h>

#include <math.h>

/*/
 * Afin que le code écrit soit facilement trouvable,
 * toutes les fonctions implémentées sont en fin de fichier.
 * Ce qui explique pourquoi get_max_etat ne soit qu'une
 * déclaration ici.
/*/
int get_max_etat( const Automate* automate );

void action_get_min_etat( const intptr_t element, void* data ){
	int * min = (int*) data;
	if( *min > element ) *min = element;
}

int get_min_etat( const Automate* automate ){
	int min = INT_MAX;

	pour_tout_element( automate->etats, action_get_min_etat, &min );

	return min;
}


int comparer_cle(const Cle *a, const Cle *b) {
	if( a->origine < b->origine )
		return -1;
	if( a->origine > b->origine )
		return 1;
	if( a->lettre < b->lettre )
		return -1;
	if( a->lettre > b->lettre )
		return 1;
	return 0;
}

void print_cle( const Cle * a){
	printf( "(%d, %c)" , a->origine, (char) (a->lettre) );
}

void supprimer_cle( Cle* cle ){
	xfree( cle );
}

void initialiser_cle( Cle* cle, int origine, char lettre ){
	cle->origine = origine;
	cle->lettre = (int) lettre;
}

Cle * creer_cle( int origine, char lettre ){
	Cle * result = xmalloc( sizeof(Cle) );
	initialiser_cle( result, origine, lettre );
	return result;
}

Cle * copier_cle( const Cle* cle ){
	return creer_cle( cle->origine, cle->lettre );
}

Automate * creer_automate(){
	Automate * automate = xmalloc( sizeof(Automate) );
	automate->etats = creer_ensemble( NULL, NULL, NULL );
	automate->alphabet = creer_ensemble( NULL, NULL, NULL );
	automate->transitions = creer_table(
		( int(*)(const intptr_t, const intptr_t) ) comparer_cle , 
		( intptr_t (*)( const intptr_t ) ) copier_cle,
		( void(*)(intptr_t) ) supprimer_cle
	);
	automate->initiaux = creer_ensemble( NULL, NULL, NULL );
	automate->finaux = creer_ensemble( NULL, NULL, NULL );
	automate->vide = creer_ensemble( NULL, NULL, NULL ); 
	return automate;
}

Automate * translater_automate_entier( const Automate* automate, int translation ){
	Automate * res = creer_automate();

	Ensemble_iterateur it;
	for( 
		it = premier_iterateur_ensemble( get_etats( automate ) );
		! iterateur_ensemble_est_vide( it );
		it = iterateur_suivant_ensemble( it )
	){
		ajouter_etat( res, get_element( it ) + translation );
	}

	for( 
		it = premier_iterateur_ensemble( get_initiaux( automate ) );
		! iterateur_ensemble_est_vide( it );
		it = iterateur_suivant_ensemble( it )
	){
		ajouter_etat_initial( res, get_element( it ) + translation );
	}

	for( 
		it = premier_iterateur_ensemble( get_finaux( automate ) );
		! iterateur_ensemble_est_vide( it );
		it = iterateur_suivant_ensemble( it )
	){
		ajouter_etat_final( res, get_element( it ) + translation );
	}

	// On ajoute les lettres
	for(
		it = premier_iterateur_ensemble( get_alphabet( automate ) );
		! iterateur_ensemble_est_vide( it );
		it = iterateur_suivant_ensemble( it )
	){
		ajouter_lettre( res, (char) get_element( it ) );
	}

	Table_iterateur it1;
	Ensemble_iterateur it2;
	for(
		it1 = premier_iterateur_table( automate->transitions );
		! iterateur_est_vide( it1 );
		it1 = iterateur_suivant_table( it1 )
	){
		Cle * cle = (Cle*) get_cle( it1 );
		Ensemble * fins = (Ensemble*) get_valeur( it1 );
		for(
			it2 = premier_iterateur_ensemble( fins );
			! iterateur_ensemble_est_vide( it2 );
			it2 = iterateur_suivant_ensemble( it2 )
		){
			int fin = get_element( it2 );
			ajouter_transition(
				res, cle->origine + translation, cle->lettre, fin + translation
			);
		}
	};

	return res;
}


void liberer_automate( Automate * automate ){
	assert( automate );
	liberer_ensemble( automate->vide );
	liberer_ensemble( automate->finaux );
	liberer_ensemble( automate->initiaux );
	pour_toute_valeur_table(
		automate->transitions, ( void(*)(intptr_t) ) liberer_ensemble
	);
	liberer_table( automate->transitions );
	liberer_ensemble( automate->alphabet );
	liberer_ensemble( automate->etats );
	xfree(automate);
}

const Ensemble * get_etats( const Automate* automate ){
	return automate->etats;
}

const Ensemble * get_initiaux( const Automate* automate ){
	return automate->initiaux;
}

const Ensemble * get_finaux( const Automate* automate ){
	return automate->finaux;
}

const Ensemble * get_alphabet( const Automate* automate ){
	return automate->alphabet;
}

void ajouter_etat( Automate * automate, int etat ){
	ajouter_element( automate->etats, etat );
}

void ajouter_lettre( Automate * automate, char lettre ){
	ajouter_element( automate->alphabet, lettre );
}

void ajouter_transition(
	Automate * automate, int origine, char lettre, int fin
){
	ajouter_etat( automate, origine );
	ajouter_etat( automate, fin );
	ajouter_lettre( automate, lettre );
	
	Cle cle;
	initialiser_cle( &cle, origine, lettre );
	Table_iterateur it = trouver_table( automate->transitions, (intptr_t) &cle );
	Ensemble * ens;
	if( iterateur_est_vide( it ) ){
		ens = creer_ensemble( NULL, NULL, NULL );
		add_table( automate->transitions, (intptr_t) &cle, (intptr_t) ens );
	}else{
		ens = (Ensemble*) get_valeur( it );
	}
	ajouter_element( ens, fin );
}

void ajouter_etat_final(
	Automate * automate, int etat_final
){
	ajouter_etat( automate, etat_final );
	ajouter_element( automate->finaux, etat_final );
}

void ajouter_etat_initial(
	Automate * automate, int etat_initial
){
	ajouter_etat( automate, etat_initial );
	ajouter_element( automate->initiaux, etat_initial );
}

const Ensemble * voisins( const Automate* automate, int origine, char lettre ){
	Cle cle;
	initialiser_cle( &cle, origine, lettre );
	Table_iterateur it = trouver_table( automate->transitions, (intptr_t) &cle );
	if( ! iterateur_est_vide( it ) ){
		return (Ensemble*) get_valeur( it );
	}else{
		return automate->vide;
	}
}

Ensemble * delta1(
	const Automate* automate, int origine, char lettre
){
	Ensemble * res = creer_ensemble( NULL, NULL, NULL );
	ajouter_elements( res, voisins( automate, origine, lettre ) );
	return res; 
}

Ensemble * delta(
	const Automate* automate, const Ensemble * etats_courants, char lettre
){
	Ensemble * res = creer_ensemble( NULL, NULL, NULL );

	Ensemble_iterateur it;
	for( 
		it = premier_iterateur_ensemble( etats_courants );
		! iterateur_ensemble_est_vide( it );
		it = iterateur_suivant_ensemble( it )
	){
		const Ensemble * fins = voisins(
			automate, get_element( it ), lettre
		);
		ajouter_elements( res, fins );
	}

	return res;
}

Ensemble * delta_star(
	const Automate* automate, const Ensemble * etats_courants, const char* mot
){
	int len = strlen( mot );
	int i;
	Ensemble * old = copier_ensemble( etats_courants );
	Ensemble * new = old;
	for( i=0; i<len; i++ ){
		new = delta( automate, old, *(mot+i) );
		liberer_ensemble( old );
		old = new;
	}
	return new;
}

void pour_toute_transition(
	const Automate* automate,
	void (* action )( int origine, char lettre, int fin, void* data ),
	void* data
){
	Table_iterateur it1;
	Ensemble_iterateur it2;
	for(
		it1 = premier_iterateur_table( automate->transitions );
		! iterateur_est_vide( it1 );
		it1 = iterateur_suivant_table( it1 )
	){
		Cle * cle = (Cle*) get_cle( it1 );
		Ensemble * fins = (Ensemble*) get_valeur( it1 );
		for(
			it2 = premier_iterateur_ensemble( fins );
			! iterateur_ensemble_est_vide( it2 );
			it2 = iterateur_suivant_ensemble( it2 )
		){
			int fin = get_element( it2 );
			action( cle->origine, cle->lettre, fin, data );
		}
	};
}

Automate* copier_automate( const Automate* automate ){
	Automate * res = creer_automate();
	Ensemble_iterateur it1;
	// On ajoute les états de l'automate
	for(
		it1 = premier_iterateur_ensemble( get_etats( automate ) );
		! iterateur_ensemble_est_vide( it1 );
		it1 = iterateur_suivant_ensemble( it1 )
	){
		ajouter_etat( res, get_element( it1 ) );
	}
	// On ajoute les états initiaux
	for(
		it1 = premier_iterateur_ensemble( get_initiaux( automate ) );
		! iterateur_ensemble_est_vide( it1 );
		it1 = iterateur_suivant_ensemble( it1 )
	){
		ajouter_etat_initial( res, get_element( it1 ) );
	}
	// On ajoute les états finaux
	for(
		it1 = premier_iterateur_ensemble( get_finaux( automate ) );
		! iterateur_ensemble_est_vide( it1 );
		it1 = iterateur_suivant_ensemble( it1 )
	){
		ajouter_etat_final( res, get_element( it1 ) );
	}
	// On ajoute les lettres
	for(
		it1 = premier_iterateur_ensemble( get_alphabet( automate ) );
		! iterateur_ensemble_est_vide( it1 );
		it1 = iterateur_suivant_ensemble( it1 )
	){
		ajouter_lettre( res, (char) get_element( it1 ) );
	}
	// On ajoute les transitions
	Table_iterateur it2;
	for(
		it2 = premier_iterateur_table( automate->transitions );
		! iterateur_est_vide( it2 );
		it2 = iterateur_suivant_table( it2 )
	){
		Cle * cle = (Cle*) get_cle( it2 );
		Ensemble * fins = (Ensemble*) get_valeur( it2 );
		for(
			it1 = premier_iterateur_ensemble( fins );
			! iterateur_ensemble_est_vide( it1 );
			it1 = iterateur_suivant_ensemble( it1 )
		){
			int fin = get_element( it1 );
			ajouter_transition( res, cle->origine, cle->lettre, fin );
		}
	}
	return res;
}

Automate * translater_automate(
	const Automate * automate, const Automate * automate_a_eviter
){
	if(
		taille_ensemble( get_etats(automate) ) == 0 ||
		taille_ensemble( get_etats(automate_a_eviter) ) == 0
	){
		return copier_automate( automate );
	}
	
	int translation = 
		get_max_etat( automate_a_eviter ) - get_min_etat( automate ) + 1; 

	return translater_automate_entier( automate, translation );
	
}

int est_une_transition_de_l_automate(
	const Automate* automate,
	int origine, char lettre, int fin
){
	return est_dans_l_ensemble( voisins( automate, origine, lettre ), fin );
}

int est_un_etat_de_l_automate( const Automate* automate, int etat ){
	return est_dans_l_ensemble( get_etats( automate ), etat );
}

int est_un_etat_initial_de_l_automate( const Automate* automate, int etat ){
	return est_dans_l_ensemble( get_initiaux( automate ), etat );
}

int est_un_etat_final_de_l_automate( const Automate* automate, int etat ){
	return est_dans_l_ensemble( get_finaux( automate ), etat );
}

int est_une_lettre_de_l_automate( const Automate* automate, char lettre ){
	return est_dans_l_ensemble( get_alphabet( automate ), lettre );
}

void print_ensemble_2( const intptr_t ens ){
	print_ensemble( (Ensemble*) ens, NULL );
}

void print_lettre( intptr_t c ){
	printf("%c", (char) c );
}

void print_automate( const Automate * automate ){
	printf("- Etats : ");
	print_ensemble( get_etats( automate ), NULL );
	printf("\n- Initiaux : ");
	print_ensemble( get_initiaux( automate ), NULL );
	printf("\n- Finaux : ");
	print_ensemble( get_finaux( automate ), NULL );
	printf("\n- Alphabet : ");
	print_ensemble( get_alphabet( automate ), print_lettre );
	printf("\n- Transitions : ");
	print_table( 
		automate->transitions,
		( void (*)( const intptr_t ) ) print_cle, 
		( void (*)( const intptr_t ) ) print_ensemble_2,
		""
	);
	printf("\n");
}

int le_mot_est_reconnu( const Automate* automate, const char* mot ){
	Ensemble * arrivee = delta_star( automate, get_initiaux(automate) , mot ); 
	
	int result = 0;

	Ensemble_iterateur it;
	for(
		it = premier_iterateur_ensemble( arrivee );
		! iterateur_ensemble_est_vide( it );
		it = iterateur_suivant_ensemble( it )
	){
		if( est_un_etat_final_de_l_automate( automate, get_element(it) ) ){
			result = 1;
			break;
		}
	}
	liberer_ensemble( arrivee );
	return result;
}

Automate * mot_to_automate( const char * mot ){
	Automate * automate = creer_automate();
	int i = 0;
	int size = strlen( mot );
	for( i=0; i < size; i++ ){
		ajouter_transition( automate, i, mot[i], i+1 );
	}
	ajouter_etat_initial( automate, 0 );
	ajouter_etat_final( automate, size );
	return automate;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
// PROJET D'INFORMATIQUE THÉORIQUE 2015-2016                                                         //
//                                                                                                   //
// Fonctions à implémenter :                                                                         //
// int get_max_etat( const Automate* automate );                                                     //
// Automate * creer_union_des_automates( const Automate * automate_1, const Automate * automate_2 ); //
// Ensemble* etats_accessibles( const Automate * automate, int etat );                               //
// Ensemble* accessibles( const Automate * automate );                                               //
// Automate *automate_accessible( const Automate * automate );                                       //
// Automate *miroir( const Automate * automate);                                                     //
// Automate * creer_automate_du_melange( const Automate* automate_1,  const Automate* automate_2 );  //
//                                                                                                   //
// Notes :                                                                                           //
// Les fonctionnalités sont purement implémentées : ne sachant quelle utilisation est censée en être //
// faite, il a été choisi de respecter strictement la demande. Par exemple, la fonction miroir ne    //
// cherche pas à supprimer les états inaccessibles. Les optimisations sont laissées à l'utilisateur. //
// Aucun zèle.                                                                                       //
// De plus, puisque nous sommes dans le module-même, nous conservons le droit d'accéder directement  //
// aux champs de la structure Automate si ceux-ci clarifient le code.                                //
///////////////////////////////////////////////////////////////////////////////////////////////////////

/*/
 * get_max_etat retourne l'etat ayant l'étiquette la plus grande.
 * Les etats sont supposés avoir des valeurs positives ou nulles.
 * Cette fonction est, sans surprise, largement inspirée de get_min_etat.
/*/
void action_get_max_etat( const intptr_t element, void* data ){
	int * max = (int*) data;
	if( *max < element ) *max = element;
}

int get_max_etat( const Automate* automate ){
	int max = INT_MIN;

	pour_tout_element( automate->etats, action_get_max_etat, &max );

	return max;
}

/*/
 * miroir renvoie un automate décrivant l'automate miroir de celui
 * donné en paramètre.
 *
 * miroir_action est une fonction auxiliare inversant le sens des transitions.
/*/
void miroir_action( int origine, char lettre, int fin, void* data ){
	Automate * nouvel_automate = (Automate*) data;

	ajouter_transition( nouvel_automate, fin, lettre, origine );
}

Automate *miroir( const Automate * automate){
	Automate * nouvel_automate = creer_automate();

	// Même alphabet et mêmes états
	nouvel_automate->alphabet = copier_ensemble( get_alphabet(automate) );
	nouvel_automate->etats = copier_ensemble( get_etats(automate) );

	// Mais états initiaux et finaux inversés
	nouvel_automate->initiaux = copier_ensemble( get_finaux(automate) );
	nouvel_automate->finaux = copier_ensemble( get_initiaux(automate) );

	// Ainsi que des transitions inversés également.
	pour_toute_transition( automate, miroir_action, nouvel_automate );

/*
	Table_iterateur it;
	for(
		it = premier_iterateur_table( automate->transitions );
		! iterateur_est_vide( it );
		it = iterateur_suivant_table( it )
	){
		Cle * cle = (Cle*) get_cle( it2 );
		Ensemble * fins = (Ensemble*) get_valeur( it2 );
		for(
			it1 = premier_iterateur_ensemble( fins );
			! iterateur_ensemble_est_vide( it1 );
			it1 = iterateur_suivant_ensemble( it1 )
		){
			int fin = get_element( it1 );
			ajouter_transition( res, cle->origine, cle->lettre, fin );
		}
	}

	Ensemble_iterateur it;
	for(
		it = premier_iterateur_ensemble( ens );
		!iterateur_ensemble_est_vide( it );
		it = iterateur_suivant_ensemble( it )
	){
		lol;
	}*/

	return nouvel_automate;
}

/*/
 * etats_accessibles retourne l'ensemble des etats pouvant être atteints depuis
 * l'état etat, quelles que soient les lettres nécessaires pour cela.
 * On utilise delta1 qui renvoie l'ensemble des etats accessibles avec une lettre
 * précise.
/*/

Ensemble* etats_accessibles( const Automate * automate, int etat ){
	Ensemble * ensemble = creer_ensemble( NULL, NULL, NULL );

	// Pour chaque lettre de l'alphabet...
	Ensemble_iterateur it;
	for(
		it = premier_iterateur_ensemble( get_alphabet(automate) );
		!iterateur_ensemble_est_vide( it );
		it = iterateur_suivant_ensemble( it )
	){
		// ...on sauvegarde les états voisins de notre état etat.
		transferer_elements_et_libere( ensemble, delta1( automate, etat, (char) get_element(it) ) );
	}
	
	return ensemble;
}

/*/
 * accessibles retourne l'ensemble des états accessibles depuis les états
 * initiaux de l'automate, c'est-à-dire l'ensemble des états de cet automate,
 * privé de ses éventuels états inutiles puisqu'inaccessibles.
/*/
Ensemble* accessibles( const Automate * automate ){
	Ensemble * ensemble = creer_ensemble( NULL, NULL, NULL );

	// Pour chaque état initial de l'automate...
	Ensemble_iterateur it;
	for(
		it = premier_iterateur_ensemble( get_initiaux(automate) );
		!iterateur_ensemble_est_vide( it );
		it = iterateur_suivant_ensemble( it )
	){
		// ...on sauvegarde les états accessibles depuis cet état.
		transferer_elements_et_libere( ensemble, etats_accessibles( automate, (int) get_element(it) ) );
	}

	return ensemble;
}

/*/
 * automate_accessible retourne un automate reconnaissant le même langage
 * que l'automate en paramètre, mais allégé de ses états inutiles.
 *
 * "Inutiles" est un terme important car il ne s'agit pas que de ses états
 * inaccessibles, mais aussi des puits (états inaccessibles de l'automate
 * miroir). Ici, cette propriété n'est pas utilisée, mais de mon avis,
 * elle devrait l'être.
/*/
void automate_accessible_action( int origine, char lettre, int fin, void * data ){
	Automate * nouvel_automate = (Automate*) data;

	if ( est_un_etat_de_l_automate( nouvel_automate, origine ) && est_un_etat_de_l_automate( nouvel_automate, fin ) )
		ajouter_transition( nouvel_automate, origine, lettre, fin );
}

Automate *automate_accessible( const Automate * automate ){
	Ensemble * ensemble = accessibles( automate );

	Automate * nouvel_automate = creer_automate();

	// Comme dit dans la description, il ne faut garder que les états utiles.
	nouvel_automate->etats = ensemble;

	// L'alphabet est le même.
	// Certes, il est possible que certaines lettres n'apparaissent plus
	// dans l'automate final, cependant, le langage qu'il reconnaît ne
	// change pas, donc son alphabet non plus.
	nouvel_automate->alphabet = copier_ensemble( get_alphabet(automate) );

	// On rajoute ensuite les états initiaux et finaux non supprimés.
	nouvel_automate->initiaux = creer_intersection_ensemble( get_etats(nouvel_automate), get_initiaux(automate) );
	nouvel_automate->finaux = creer_intersection_ensemble( get_etats(nouvel_automate), get_finaux(automate) );

	// Ne restent plus que les transitions.
	// On ne garde que celles ayant pour origine et fin un état conservé.
	pour_toute_transition( automate, automate_accessible_action, nouvel_automate );

	return nouvel_automate;
}

/*/
 * 
/*/
void creer_union_des_automates_action( int origine, char lettre, int fin, void * data ){
	Automate * automate_final = (Automate*) data;

	ajouter_transition( automate_final, origine, lettre, fin );
}

Automate * creer_union_des_automates( const Automate * automate_1, const Automate * automate_2 ){
	Automate * nouvel_automate_1 = translater_automate( automate_1, automate_2 );
	Automate * automate_final = creer_automate();

	automate_final->etats = creer_union_ensemble( get_etats(nouvel_automate_1), get_etats(automate_2) );
	automate_final->alphabet = creer_union_ensemble( get_alphabet(nouvel_automate_1), get_alphabet(automate_2) );
	automate_final->initiaux = creer_union_ensemble( get_initiaux(nouvel_automate_1), get_initiaux(automate_2) );
	automate_final->finaux = creer_union_ensemble( get_finaux(nouvel_automate_1), get_finaux(automate_2) );

	pour_toute_transition( nouvel_automate_1, creer_union_des_automates_action, automate_final );
	pour_toute_transition( automate_2, creer_union_des_automates_action, automate_final );

	liberer_automate( nouvel_automate_1 );

	return automate_final;
}

/*/
 * 
/*/
Automate * creer_automate_du_melange(
	const Automate* automate_1,  const Automate* automate_2
){
	A_FAIRE_RETURN( NULL ); 
}