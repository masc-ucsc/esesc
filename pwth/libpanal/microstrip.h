#ifdef __cplusplus
extern "C" {
#endif/* Parasitic Cap, Resistance for metal lines  
 */

/* typical 4 layer PCB parameters 
** Courtesy of www.icd.com.au/layere4.html
*/
#define MICROSTRIP_ER 4.7 
#define MICROSTRIP_H 16 
#define MICROSTRIP_T 0.7
#define MICROSTRIP_W 7
 
/* Buffer delay for typical buffer load*/
#define BUFFER_DELAY 80 /* ps*/ 


typedef struct _microstrip_t {
	double dielectric;
	double width;
	double thickness;
	double height;
	double length;
} microstrip_t;

extern double estimate_microstrip_capacitance(microstrip_t * spec);

#ifdef __cplusplus
}
#endif

