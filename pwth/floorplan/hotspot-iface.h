#ifndef __HOTSPOT_IFACE_H_
#define __HOTSPOT_IFACE_H_

#define STR_SIZE	512
#define MAX_UNITS	8192
#define NULLFILE	"(null)"
#define BLOCK_MODEL		0
#define GRID_MODEL		1

/* floorplan structure	*/
typedef struct unit_t_st
{
	char name[STR_SIZE];
	double width;
	double height;
	double leftx;
	double bottomy;
}unit_t;
typedef struct flp_t_st
{
	unit_t *units;
	int n_units;
  	double **wire_density;
} flp_t;

/* floorplan routines	*/

/* reads the floorplan from a file and allocates memory. 
 * 'read_connects' is a boolean flag indicating if 
 * connectivity information should also be read (usually 
 * set to FALSE).
 */ 
flp_t *read_flp(char *file, int read_connects);

/* deletes floorplan from memory. 'compacted' is a 
 * boolean flag indicating if this is a floorplan
 * compacted by HotFloorplan (usually set to FALSE).
 */ 
void free_flp(flp_t *flp, int compacted);

/* thermal model configuration structure	*/
typedef struct thermal_config_t_st
{
	double t_chip;
	double thermal_threshold;
	double c_convec;
	double r_convec;
	double s_sink;
	double t_sink;
	double s_spreader;
	double t_spreader;
	double t_interface;
	double ambient;
	char init_file[STR_SIZE];
	double init_temp;
	char steady_file[STR_SIZE];
	double sampling_intvl;
	double base_proc_freq;
	int dtm_used;
	char model_type[STR_SIZE];
	int block_omit_lateral;
	int grid_rows;
	int grid_cols;
	char grid_layer_file[STR_SIZE];
	char grid_steady_file[STR_SIZE];
	char grid_map_mode[STR_SIZE];
}thermal_config_t;

/* thermal configuration routines */

/* returns a thermal configuration structure with
 * the default set of parameters
 */ 
thermal_config_t default_thermal_config(void);

/* thermal model structure	*/
struct block_model_t_st;
struct grid_model_t_st;
typedef struct RC_model_t_st
{
	union
	{
		struct block_model_t_st *block;
		struct grid_model_t_st *grid;
	};
	int type;
	thermal_config_t *config;
}RC_model_t;

/* thermal model routines */

/* creates a new thermal model. 'config' can be obtained 
 * from the 'default_thermal_config' function and 
 * 'placeholder' can be obtained from the 'read_flp' 
 * function
 */ 
RC_model_t *alloc_RC_model(thermal_config_t *config, flp_t *placeholder);

/* deletes the thermal model and frees up memory	*/
void delete_RC_model(RC_model_t *model);

/* populates the thermal resistances of the model. This is
 * a prerequisite for computing transient or steady state
 * temperatures.
 */ 
void populate_R_model(RC_model_t *model, flp_t *flp);

/* populates the thermal capacitances of the model. This is
 * a prerequisite for computing transient temperatures.
 */ 
void populate_C_model(RC_model_t *model, flp_t *flp);

/* memory allocator for the power and temperature vectors	*/
double *hotspot_vector(RC_model_t *model);

/* destructor for a vector allocated using 'hotspot_vector'	*/
void free_dvector(double *v);

/* outputs the 'temp' vector onto 'file'. 'temp' must
 * be allocated using ' hotspot_vector'.
 */
void dump_temp (RC_model_t *model, double *temp, char *file);

/* sets all the temperatures of the 'temp' vector to the
 * value 'val'. 'temp' must be allocated using '
 * hotspot_vector'.
 */
void set_temp (RC_model_t *model, double *temp, double val);

/* read the 'temp' vector from 'file'. The format of the 
 * file should be the same as the one output by the 
 * 'dump_temp' function. 'temp' must be allocated using 
 * 'hotspot_vector'. 'clip' is a boolean flag indicating
 * whether to clip the peak temperature of the vector to
 * the thermal threshold 'model->config->thermal_threshold'
 * (usually set to FALSE).
 */
void read_temp (RC_model_t *model, double *temp, char *file, int clip);

/* computation of the steady state temperatures. 'power'
 * and 'temp' must be allocated using 'hotspot_vector'.
 * 'populate_R_model' must be called before this.
 * 'power' should contain the input power numbers. 'temp'
 * will contain the output temperature numbers after the 
 * call.
 */ 
void steady_state_temp(RC_model_t *model, double *power, double *temp);

/* computation of the transient temperatures. 'power'
 * and 'temp' must be allocated using 'hotspot_vector'.
 * 'populate_R_model' and 'populate_C_model' must be
 * called before this. 'power' should  contain the 
 * input power numbers and 'temp' should contain the 
 * current temperatures. 'time_elapsed' is the duration
 * of the transient simulation. 'temp' will contain the
 * output temperature numbers after the call.
 */ 
void compute_temp(RC_model_t *model, double *power, double *temp, double time_elapsed);

#endif
