#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _MSC_VER
#define strcasecmp    _stricmp
#define strncasecmp   _strnicmp
#else
#include <strings.h>
#endif
#include <math.h>

#include "temperature_grid.h"
#include "flp.h"
#include "util.h"

/* constructors	*/
blist_t *new_blist(int idx, double occupancy)
{
	blist_t *ptr = (blist_t *) calloc (1, sizeof(blist_t));
	if (!ptr)
		fatal("memory allocation error\n");
	ptr->idx = idx;
	ptr->occupancy = occupancy;
	ptr->next = NULL;
	return ptr;
}

blist_t ***new_b2gmap(int rows, int cols)
{
	int i;
	blist_t ***b2gmap;

	b2gmap = (blist_t ***) calloc (rows, sizeof(blist_t **));
	b2gmap[0] = (blist_t **) calloc (rows * cols, sizeof(blist_t *));
	if (!b2gmap || !b2gmap[0])
		fatal("memory allocation error\n");

	for(i=1; i < rows; i++) 
		b2gmap[i] = b2gmap[0] + cols * i;

	return b2gmap;	
}

/* destructor	*/
void delete_b2gmap(blist_t ***b2gmap, int rows, int cols)
{
	int i, j;
	blist_t *ptr, *temp;

	/* free the linked list	*/
	for(i=0; i < rows; i++)
		for(j=0; j < cols; j++) {
			ptr = b2gmap[i][j];
			while(ptr) {
				temp = ptr->next;
				free(ptr);
				ptr = temp;
			}
		}

	/* free the array space	*/
	free(b2gmap[0]);
	free(b2gmap);
}

/* re-initialize */
void reset_b2gmap(grid_model_t *model, layer_t *layer)
{
	int i, j;
	blist_t *ptr, *temp;

	/* free the linked list	*/
	for(i=0; i < model->rows; i++)
		for(j=0; j < model->cols; j++) {
			ptr = layer->b2gmap[i][j];
			while(ptr) {
				temp = ptr->next;
				free(ptr);
				ptr = temp;
			}
			layer->b2gmap[i][j] = NULL;
		}
}

/* create a linked list node and append it at the end	*/
void blist_append(blist_t *head, int idx, double occupancy)
{
	blist_t *tail = NULL;
	
	if(!head)
		fatal("blist_append called with empty list\n");

	/* traverse till the end	*/
	for(; head; head = head->next)
		tail = head;

	/* append */
	tail->next =  new_blist(idx, occupancy);
}

/* compute the power/temperature average weighted by occupancies	*/
double blist_avg(blist_t *ptr, flp_t *flp, double *v, int type)
{
	double  val = 0.0;
	
	for(; ptr; ptr = ptr->next) {
		if (type == V_POWER)
			val += ptr->occupancy * v[ptr->idx] / (flp->units[ptr->idx].width * 
				   flp->units[ptr->idx].height);
		else if (type == V_TEMP)		   
			val += ptr->occupancy * v[ptr->idx];
		else
			fatal("unknown vector type\n");
	}		

	return val;		   
}

void debug_print_blist(blist_t *head, flp_t *flp);
/* test the block-grid map data structure	*/
void test_b2gmap(grid_model_t *model, layer_t *layer)
{
	int i, j;
	blist_t *ptr;
	double sum;

	/* a correctly formed b2gmap should have the 
	 * sum of occupancies in each linked list
	 * to be equal to 1.0
	 */
	for (i=0; i < model->rows; i++)
		for(j=0; j < model->cols; j++) {
			sum = 0.0;
			for(ptr = layer->b2gmap[i][j]; ptr; ptr = ptr->next)
				sum += ptr->occupancy;
			if (!eq(floor(sum*1e5 + 0.5)/1e5, 1.0)) {
				fprintf(stdout, "i: %d\tj: %d\n", i, j);
				debug_print_blist(layer->b2gmap[i][j], layer->flp);
				fatal("erroneous b2gmap data structure. invalid floorplan?\n");
			}	
		}
}

/* setup the block and grid mapping data structures	*/
void set_bgmap(grid_model_t *model, layer_t *layer)
{
	/* i1, i2, j1 and j2 are indices of the boundary grid cells	*/
	int i, j, u, i1, i2, j1, j2;

	/* shortcuts for cell width(cw) and cell height(ch)	*/
	double cw = model->width / model->cols;
	double ch = model->height / model->rows;

	/* initialize	*/
	reset_b2gmap(model, layer);

	/* for each functional unit	*/
	for(u=0; u < layer->flp->n_units; u++) {
		/* shortcuts for unit boundaries	*/
		double lu = layer->flp->units[u].leftx;
		double ru = lu + layer->flp->units[u].width;
		double bu = layer->flp->units[u].bottomy;
		double tu = bu + layer->flp->units[u].height;

		/* top index (lesser row) = rows - ceil (topy / cell height)	*/
		i1 = model->rows - tolerant_ceil(tu/ch);
		/* bottom index (greater row) = rows - floor (bottomy / cell height)	*/
		i2 = model->rows - tolerant_floor(bu/ch);
		/* left index = floor (leftx / cell width)	*/
#ifndef CONFIG_PB
		j1 = tolerant_floor(lu/cw);
#else
		j1 = round(lu/cw);
#endif
		/* right index = ceil (rightx / cell width)	*/
		j2 = tolerant_ceil(ru/cw);
#ifdef CONFIG_PB
    if (j1 == j2)
      j1--;
#endif
    //printf("b[%d]:[%d-%d][%d-%d] [%f-%f][%f-%f]\t",u, i1,i2,j1,j2, lu/cw,ru/cw,bu/ch,tu/ch);
		/* sanity check	*/
		if((i1 < 0) || (j1 < 0))
			fatal("negative grid cell start index!\n");
		if((i2 > model->rows) || (j2 > model->cols))
			fatal("grid cell end index out of bounds!\n");
		if((i1 >= i2) || (j1 >= j2))
			fatal("invalid floorplan spec or grid resolution\n");

		/* setup g2bmap	*/
		layer->g2bmap[u].i1 = i1;
		layer->g2bmap[u].i2 = i2;
		layer->g2bmap[u].j1 = j1;
		layer->g2bmap[u].j2 = j2;

    /* setup b2gmap	*/
		/* for each grid cell in this unit	*/
		for(i=i1; i < i2; i++)
			for(j=j1; j < j2; j++)
				/* grid cells fully overlapped by this unit	*/
				if ((i > i1) && (i < i2-1) && (j > j1) && (j < j2-1)) {
					/* first unit in the list	*/
					if (!layer->b2gmap[i][j]){
						layer->b2gmap[i][j] = new_blist(u, 1.0);
      //      printf("g[%d][%d]:[%d]\t", i,j,u);
          }
					else {
					/* this should not occur since the grid cell is 
					 * fully covered and hence, no other unit should 
					 * be sharing it
					 */
						blist_append(layer->b2gmap[i][j], u, 1.0);
						warning("overlap of functional blocks?\n");
					}
				/* boundary grid cells partially overlapped by this unit	*/
				} else {
					/* shortcuts for cell boundaries	*/
					double lc = j * cw, rc = (j+1) * cw;
					double tc = model->height - i * ch;
					double bc = model->height - (i+1) * ch;
					
					/* shortcuts for overlap width and height	*/
					double oh = (MIN(tu, tc) - MAX(bu, bc));
					double ow = (MIN(ru, rc) - MAX(lu, lc));
					double occupancy;
			
					/* overlap tolerance	*/
					if (eq(oh/ch, 0))
						oh = 0;
					else if (eq(oh/ch, 1))
						oh = ch;

					if (eq(ow/cw, 0))
						ow = 0;
					else if (eq(ow/cw, 1))
						ow = cw;

					occupancy = (oh * ow) / (ch * cw);
					if (oh < 0 || ow < 0)
						fatal("negative overlap!\n");

        //    printf("g[%d][%d]:[%d]\t", i,j,u);
					/* first unit in the list	*/
					if (!layer->b2gmap[i][j])
						layer->b2gmap[i][j] = new_blist(u, occupancy);
					else
					/* append at the end	*/
						blist_append(layer->b2gmap[i][j], u, occupancy);
				}
	}

	/* 
	 * sanity check	
	test_b2gmap(model, layer);
	 */
}

/* populate default set of layers	*/ 
void populate_default_layers(grid_model_t *model, flp_t *flp_default)
{
	/* silicon	*/
	model->layers[LAYER_SI].no = LAYER_SI;
	model->layers[LAYER_SI].has_lateral = TRUE;
	model->layers[LAYER_SI].has_power = TRUE;
	model->layers[LAYER_SI].k = model->config.k_chip;
	model->layers[LAYER_SI].thickness = model->config.t_chip;
	model->layers[LAYER_SI].sp = model->config.p_chip;
	model->layers[LAYER_SI].flp = flp_default;
	model->layers[LAYER_SI].b2gmap = new_b2gmap(model->rows, model->cols);
	model->layers[LAYER_SI].g2bmap = (glist_t *) calloc(flp_default->n_units, sizeof(glist_t));
	if (!model->layers[LAYER_SI].g2bmap)
		fatal("memory allocation error\n");

	/* interface material	*/
	model->layers[LAYER_INT].no = LAYER_INT;
	model->layers[LAYER_INT].has_lateral = TRUE;
	model->layers[LAYER_INT].has_power = FALSE;
	model->layers[LAYER_INT].k = model->config.k_interface;
	model->layers[LAYER_INT].thickness = model->config.t_interface;
	model->layers[LAYER_INT].sp = model->config.p_interface;
	model->layers[LAYER_INT].flp = flp_default;
	model->layers[LAYER_INT].b2gmap = model->layers[LAYER_SI].b2gmap;
	model->layers[LAYER_INT].g2bmap = model->layers[LAYER_SI].g2bmap;
	
	if (model->config.model_secondary) {
		/* metal layer	*/
		model->layers[DEFAULT_CHIP_LAYERS + LAYER_METAL].no = DEFAULT_CHIP_LAYERS + LAYER_METAL;
		model->layers[DEFAULT_CHIP_LAYERS + LAYER_METAL].has_lateral = TRUE;
		model->layers[DEFAULT_CHIP_LAYERS + LAYER_METAL].has_power = FALSE;
		model->layers[DEFAULT_CHIP_LAYERS + LAYER_METAL].k = K_METAL;
		model->layers[DEFAULT_CHIP_LAYERS + LAYER_METAL].k1 = K_DIELECTRIC; 
		model->layers[DEFAULT_CHIP_LAYERS + LAYER_METAL].thickness = model->config.t_metal;
		model->layers[DEFAULT_CHIP_LAYERS + LAYER_METAL].sp = SPEC_HEAT_METAL;
		model->layers[DEFAULT_CHIP_LAYERS + LAYER_METAL].flp = flp_default;
		model->layers[DEFAULT_CHIP_LAYERS + LAYER_METAL].b2gmap = model->layers[LAYER_SI].b2gmap;
		model->layers[DEFAULT_CHIP_LAYERS + LAYER_METAL].g2bmap = model->layers[LAYER_SI].g2bmap;
		if (!model->layers[DEFAULT_CHIP_LAYERS + LAYER_METAL].g2bmap)
			fatal("memory allocation error\n");
  	
		/* C4/underfill layer*/
		model->layers[DEFAULT_CHIP_LAYERS + LAYER_C4].no = DEFAULT_CHIP_LAYERS + LAYER_C4;
		model->layers[DEFAULT_CHIP_LAYERS + LAYER_C4].has_lateral = TRUE;
		model->layers[DEFAULT_CHIP_LAYERS + LAYER_C4].has_power = FALSE;
		model->layers[DEFAULT_CHIP_LAYERS + LAYER_C4].k = K_C4;
		model->layers[DEFAULT_CHIP_LAYERS + LAYER_C4].k1 = K_UNDERFILL;
		model->layers[DEFAULT_CHIP_LAYERS + LAYER_C4].thickness = model->config.t_c4;
		model->layers[DEFAULT_CHIP_LAYERS + LAYER_C4].sp = SPEC_HEAT_C4;
		model->layers[DEFAULT_CHIP_LAYERS + LAYER_C4].sp1 = SPEC_HEAT_UNDERFILL;
		model->layers[DEFAULT_CHIP_LAYERS + LAYER_C4].flp = flp_default;
		model->layers[DEFAULT_CHIP_LAYERS + LAYER_C4].b2gmap = model->layers[LAYER_SI].b2gmap;
		model->layers[DEFAULT_CHIP_LAYERS + LAYER_C4].g2bmap = model->layers[LAYER_SI].g2bmap;
	}
}

/* populate the package layers	*/ 
void append_package_layers(grid_model_t *model)
{
	/* shortcut	*/
	int nl = model->n_layers;

	/* spreader	*/
	model->layers[nl+LAYER_SP].no = nl+LAYER_SP;
	model->layers[nl+LAYER_SP].has_lateral = TRUE;
	model->layers[nl+LAYER_SP].has_power = FALSE;
	model->layers[nl+LAYER_SP].k = model->config.k_spreader;
	model->layers[nl+LAYER_SP].thickness = model->config.t_spreader;
	model->layers[nl+LAYER_SP].sp = model->config.p_spreader;
	model->layers[nl+LAYER_SP].flp = model->layers[nl-1].flp;
	model->layers[nl+LAYER_SP].b2gmap = model->layers[nl-1].b2gmap;
	model->layers[nl+LAYER_SP].g2bmap = model->layers[nl-1].g2bmap;

	/* heatsink	*/
	model->layers[nl+LAYER_SINK].no = nl+LAYER_SINK;
	model->layers[nl+LAYER_SINK].has_lateral = TRUE;
	model->layers[nl+LAYER_SINK].has_power = FALSE;
	model->layers[nl+LAYER_SINK].k = model->config.k_sink;
	model->layers[nl+LAYER_SINK].thickness = model->config.t_sink;
	model->layers[nl+LAYER_SINK].sp = model->config.p_sink;
	model->layers[nl+LAYER_SINK].flp = model->layers[nl-1].flp;
	model->layers[nl+LAYER_SINK].b2gmap = model->layers[nl-1].b2gmap;
	model->layers[nl+LAYER_SINK].g2bmap = model->layers[nl-1].g2bmap;

	model->n_layers += DEFAULT_PACK_LAYERS;
		
	if (model->config.model_secondary) {
		/* package substrate	*/
		model->layers[nl+DEFAULT_PACK_LAYERS+LAYER_SUB].no = nl+DEFAULT_PACK_LAYERS+LAYER_SUB;
		model->layers[nl+DEFAULT_PACK_LAYERS+LAYER_SUB].has_lateral = TRUE;
		model->layers[nl+DEFAULT_PACK_LAYERS+LAYER_SUB].has_power = FALSE;
		model->layers[nl+DEFAULT_PACK_LAYERS+LAYER_SUB].k = K_SUB;
		model->layers[nl+DEFAULT_PACK_LAYERS+LAYER_SUB].thickness = model->config.t_sub;
		model->layers[nl+DEFAULT_PACK_LAYERS+LAYER_SUB].sp = SPEC_HEAT_SUB;
		model->layers[nl+DEFAULT_PACK_LAYERS+LAYER_SUB].flp = model->layers[nl-1].flp;
		model->layers[nl+DEFAULT_PACK_LAYERS+LAYER_SUB].b2gmap = model->layers[nl-1].b2gmap;
		model->layers[nl+DEFAULT_PACK_LAYERS+LAYER_SUB].g2bmap = model->layers[nl-1].g2bmap;
		
		/* solder balls	*/
		model->layers[nl+DEFAULT_PACK_LAYERS+LAYER_SOLDER].no = nl+DEFAULT_PACK_LAYERS+LAYER_SOLDER;
		model->layers[nl+DEFAULT_PACK_LAYERS+LAYER_SOLDER].has_lateral = TRUE;
		model->layers[nl+DEFAULT_PACK_LAYERS+LAYER_SOLDER].has_power = FALSE;
		model->layers[nl+DEFAULT_PACK_LAYERS+LAYER_SOLDER].k = K_SOLDER;
		model->layers[nl+DEFAULT_PACK_LAYERS+LAYER_SOLDER].thickness = model->config.t_solder;
		model->layers[nl+DEFAULT_PACK_LAYERS+LAYER_SOLDER].sp = SPEC_HEAT_SOLDER;
		model->layers[nl+DEFAULT_PACK_LAYERS+LAYER_SOLDER].flp = model->layers[nl-1].flp;
		model->layers[nl+DEFAULT_PACK_LAYERS+LAYER_SOLDER].b2gmap = model->layers[nl-1].b2gmap;
		model->layers[nl+DEFAULT_PACK_LAYERS+LAYER_SOLDER].g2bmap = model->layers[nl-1].g2bmap;
  	
		/* PCB	*/
		model->layers[nl+DEFAULT_PACK_LAYERS+LAYER_PCB].no = nl+DEFAULT_PACK_LAYERS+LAYER_PCB;
		model->layers[nl+DEFAULT_PACK_LAYERS+LAYER_PCB].has_lateral = TRUE;
		model->layers[nl+DEFAULT_PACK_LAYERS+LAYER_PCB].has_power = FALSE;
		model->layers[nl+DEFAULT_PACK_LAYERS+LAYER_PCB].k = K_PCB;
		model->layers[nl+DEFAULT_PACK_LAYERS+LAYER_PCB].thickness = model->config.t_pcb;
		model->layers[nl+DEFAULT_PACK_LAYERS+LAYER_PCB].sp = SPEC_HEAT_PCB;
		model->layers[nl+DEFAULT_PACK_LAYERS+LAYER_PCB].flp = model->layers[nl-1].flp;
		model->layers[nl+DEFAULT_PACK_LAYERS+LAYER_PCB].b2gmap = model->layers[nl-1].b2gmap;
		model->layers[nl+DEFAULT_PACK_LAYERS+LAYER_PCB].g2bmap = model->layers[nl-1].g2bmap;
		
		model->n_layers += SEC_PACK_LAYERS;		
	}
}

/* parse the layer file open for reading	*/
void parse_layer_file(grid_model_t *model, FILE *fp)
{
	char line[LINE_SIZE], *ptr, cval;
	int count, i = 0, field = LCF_SNO, ival;
	double dval;

	fseek(fp, 0, SEEK_SET);
	count = 0;
	while (!feof(fp) && count < (model->n_layers * LCF_NPARAMS)) {
		fgets(line, LINE_SIZE, fp);
		if (feof(fp))
			break;

		/* ignore comments and empty lines	*/
		ptr = strtok(line, " \r\t\n");
		if (!ptr || ptr[0] == '#')
			continue;
			
		switch (field) 
		{
			case LCF_SNO:
						if (sscanf(ptr, "%d", &ival) != 1)
							fatal("invalid layer number\n");
						if(ival >= model->n_layers || ival < 0)
							fatal("layer number must be >= 0 and < no. of layers\n");
						if (model->layers[ival].no != 0)
							fatal("layer numbers must be unique\n");
						i = ival;
						model->layers[i].no = ival;
						break;
			case LCF_LATERAL:
						if (sscanf(ptr, "%c", &cval) != 1)
							fatal("invalid layer heat flow indicator\n");
						if (cval == 'Y' || cval == 'y')
							model->layers[i].has_lateral = TRUE;
						else if (cval == 'N' || cval == 'n')			
							model->layers[i].has_lateral = FALSE;
						else			
							fatal("invalid layer heat flow indicator\n");
						break;
			case LCF_POWER:
						if (sscanf(ptr, "%c", &cval) != 1)
							fatal("invalid layer power dissipation indicator\n");
						if (cval == 'Y' || cval == 'y')
							model->layers[i].has_power = TRUE;
						else if (cval == 'N' || cval == 'n')			
							model->layers[i].has_power = FALSE;
						else			
							fatal("invalid layer power dissipation indicator\n");
						break;
			case LCF_SP:
						if (sscanf(ptr, "%lf", &dval) != 1)
							fatal("invalid specific heat\n");
						model->layers[i].sp = dval;
						break;
			case LCF_RHO:
						if (sscanf(ptr, "%lf", &dval) != 1)
							fatal("invalid resistivity\n");
						model->layers[i].k = 1.0 / dval;
						break;
			case LCF_THICK:
						if (sscanf(ptr, "%lf", &dval) != 1)
							fatal("invalid thickness\n");
						model->layers[i].thickness = dval;
						break;
			case LCF_FLP:
						model->layers[i].flp = read_flp(ptr, FALSE);
						/* first layer	*/
						if (count < LCF_NPARAMS) {
							model->width = get_total_width(model->layers[i].flp);
							model->height = get_total_height(model->layers[i].flp);
						} else if(!eq(model->width, get_total_width(model->layers[i].flp)) || 
								  !eq(model->height, get_total_height(model->layers[i].flp)))
							fatal("width and height differ across layers\n");
						break;
			default:
						fatal("invalid field id\n");
						break;
		}
		field = (field + 1) % LCF_NPARAMS;
		count++;
	}

	/* allocate the block-grid maps */
	for(i=0; i < model->n_layers; i++) {
		model->layers[i].b2gmap = new_b2gmap(model->rows, model->cols);
		model->layers[i].g2bmap = (glist_t *) calloc(model->layers[i].flp->n_units, 
								   sizeof(glist_t));
		if (!model->layers[i].g2bmap)
			fatal("memory allocation error\n");
	}
}

/* populate layer info either from the default floorplan or from
 * the layer configuration file (lcf)
 */
void populate_layers_grid(grid_model_t *model, flp_t *flp_default)
{
	char str[STR_SIZE];
	FILE *fp = NULL;

	/* lcf file specified	*/
	if (model->has_lcf) {
		if (!strcasecmp(model->config.grid_layer_file, "stdin"))
			fp = stdin;
		else
			fp = fopen (model->config.grid_layer_file, "r");
		if (!fp) {
			sprintf(str, "error opening file %s\n", model->config.grid_layer_file);
			fatal(str);
		}
	}

	/* compute the no. of layers	*/
	if (!model->config.model_secondary) {
		if (model->has_lcf) {
			model->n_layers = count_significant_lines(fp);
			if (model->n_layers % LCF_NPARAMS)
				fatal("wrong no. of lines in layer file\n");
			model->n_layers /= LCF_NPARAMS;
		/* default no. of layers when lcf file is not specified	*/	
		} else
			model->n_layers = DEFAULT_CHIP_LAYERS;
	} else {
		if (model->has_lcf) {
			model->n_layers = count_significant_lines(fp);
			if (model->n_layers % LCF_NPARAMS)
				fatal("wrong no. of lines in layer file\n");
			model->n_layers /= LCF_NPARAMS;
			model->n_layers += SEC_CHIP_LAYERS;
		/* default no. of layers when lcf file is not specified	*/	
		} else
			model->n_layers = DEFAULT_CHIP_LAYERS + SEC_CHIP_LAYERS;
	}

	/* allocate initial memory including package layers	*/
	if (!model->config.model_secondary) {
		model->layers = (layer_t *) calloc (model->n_layers + DEFAULT_PACK_LAYERS, sizeof(layer_t));
		if (!model->layers)
			fatal("memory allocation error\n");
	} else {
		model->layers = (layer_t *) calloc (model->n_layers + DEFAULT_PACK_LAYERS + SEC_PACK_LAYERS, sizeof(layer_t));
		if (!model->layers)
			fatal("memory allocation error\n");
	}

	/* read in values from the lcf when specified	*/
	if (model->has_lcf) {
		if (model->config.model_secondary)
			fatal("modeling secondary heat transfer path not supported when layer configuration file specified...\n");
		parse_layer_file(model, fp);
		warning("layer configuration file specified. overriding default floorplan with those in lcf file...\n");
	/* default set of layers	*/
	} else
		populate_default_layers(model, flp_default);
	
	/* append the package layers	*/
	append_package_layers(model);

	if (model->has_lcf && fp != stdin)
		fclose(fp);
}

/* constructor	*/ 
grid_model_t *alloc_grid_model(thermal_config_t *config, flp_t *flp_default)
{
	int i;
	grid_model_t *model;

	if (config->grid_rows & (config->grid_rows-1) ||
		config->grid_cols & (config->grid_cols-1))
		fatal("grid rows and columns should both be powers of two\n");

	model = (grid_model_t *) calloc (1, sizeof(grid_model_t));
	if (!model)
		fatal("memory allocation error\n");
	model->config = *config;
	model->rows = config->grid_rows;
	model->cols = config->grid_cols;

	if(!strcasecmp(model->config.grid_map_mode, GRID_AVG_STR))
		model->map_mode = GRID_AVG;
	else if(!strcasecmp(model->config.grid_map_mode, GRID_MIN_STR))
		model->map_mode = GRID_MIN;
	else if(!strcasecmp(model->config.grid_map_mode, GRID_MAX_STR))
		model->map_mode = GRID_MAX;
	else if(!strcasecmp(model->config.grid_map_mode, GRID_CENTER_STR))
		model->map_mode = GRID_CENTER;
	else
		fatal("unknown mapping mode\n");

	/* layer configuration file specified?	*/
	if(strcmp(model->config.grid_layer_file, NULLFILE))
		model->has_lcf = TRUE;
	else {
		model->has_lcf = FALSE;
		model->base_n_units = flp_default->n_units;
	}

	/* get layer information	*/
	populate_layers_grid(model, flp_default);

	/* count the total no. of blocks */
	model->total_n_blocks = 0;
	for(i=0; i < model->n_layers; i++)
		model->total_n_blocks += model->layers[i].flp->n_units;

	/* allocate internal state	*/
	model->last_steady = new_grid_model_vector(model);
	model->last_trans = new_grid_model_vector(model);
	
	return model;
}

void populate_R_model_grid(grid_model_t *model, flp_t *flp)
{
	int i;
	double cw, ch;
	
	int inner_layers;
	int model_secondary = model->config.model_secondary;
	
	if (model_secondary)
		inner_layers = model->n_layers - DEFAULT_PACK_LAYERS - SEC_PACK_LAYERS;
	else
		inner_layers = model->n_layers - DEFAULT_PACK_LAYERS;

	/* setup the block-grid maps; flp parameter is ignored */
	if (model->has_lcf)
		for(i=0; i < inner_layers; i++)
			set_bgmap(model, &model->layers[i]);
	/* only the silicon layer has allocated space for the maps. 
	 * all the rest just point to it. so it is sufficient to
	 * setup the block-grid map for the silicon layer alone.
	 * further, for default layer configuration, the `flp' 
	 * parameter should be the same as that of the silicon 
	 * layer. finally, the chip width and height information 
	 * need to be populated for default layer configuration
	 */
	else {
		if (flp != model->layers[LAYER_SI].flp)
			fatal("mismatch between the floorplan and the thermal model\n");
		model->width = get_total_width(flp);
		model->height = get_total_height(flp);
		set_bgmap(model, &model->layers[LAYER_SI]);
	}
	/* sanity check on floorplan sizes	*/
	if (model->width > model->config.s_sink || 
		model->height > model->config.s_sink || 
		model->width > model->config.s_spreader || 
		model->height > model->config.s_spreader) {
		print_flp(model->layers[0].flp);
		print_flp_fig(model->layers[0].flp);
		fatal("inordinate floorplan size!\n");
	}

	/* shortcuts for cell width(cw) and cell height(ch)	*/
	cw = model->width / model->cols;
	ch = model->height / model->rows;

	/* package R's	*/
	populate_package_R(&model->pack, &model->config, model->width, model->height);

	/* layer specific resistances	*/
	for(i=0; i < model->n_layers; i++) {
		if (model->layers[i].has_lateral) {
			model->layers[i].rx =  getr(model->layers[i].k, cw, ch * model->layers[i].thickness);
			model->layers[i].ry =  getr(model->layers[i].k, ch, cw * model->layers[i].thickness);
			if (model_secondary && (i==inner_layers-SEC_CHIP_LAYERS+LAYER_C4)) {
				model->layers[i].rx1 =  getr(model->layers[i].k1, cw, ch * model->layers[i].thickness);
				model->layers[i].ry1 =  getr(model->layers[i].k1, ch, cw * model->layers[i].thickness);
			}
		} else {
			/* positive infinity	*/
			model->layers[i].rx = LARGENUM;
			model->layers[i].ry = LARGENUM;
			if (model_secondary && (i==inner_layers-SEC_CHIP_LAYERS+LAYER_C4)) {
				model->layers[i].rx1 = LARGENUM;
				model->layers[i].ry1 = LARGENUM;
			}
		}
		model->layers[i].rz =  getr(model->layers[i].k, model->layers[i].thickness, cw * ch);
		if (model_secondary && (i==inner_layers-SEC_CHIP_LAYERS+LAYER_C4)) {
			model->layers[i].rz1 =  getr(model->layers[i].k1, model->layers[i].thickness, cw * ch);
		}
		
		if (!model_secondary) {
			/* heatsink	is connected to ambient. divide r_convec proportional to cell area */
			if (i == model->n_layers - DEFAULT_PACK_LAYERS + LAYER_SINK)
				model->layers[i].rz += model->config.r_convec * 
								   (model->config.s_sink * model->config.s_sink) / (cw * ch);
		} else {
			/* heatsink	is connected to ambient. divide r_convec proportional to cell area */
			if (i == model->n_layers - DEFAULT_PACK_LAYERS - SEC_PACK_LAYERS + LAYER_SINK)
				model->layers[i].rz += model->config.r_convec * 
								   (model->config.s_sink * model->config.s_sink) / (cw * ch);\
			/* PCB	is connected to ambient. divide r_convec_sec proportional to cell area */
//			if (i == model->n_layers - SEC_PACK_LAYERS + LAYER_PCB)
//				model->layers[i].rz += model->config.r_convec_sec * 
//								   (model->config.s_pcb * model->config.s_pcb) / (cw * ch);
		}
	}

	/* done	*/
	model->r_ready = TRUE;
}

void populate_C_model_grid(grid_model_t *model, flp_t *flp)
{
	int i;
	
	int inner_layers;
	int model_secondary = model->config.model_secondary;
	
	/* shortcuts for cell width(cw) and cell height(ch)	*/
	double cw = model->width / model->cols;
	double ch = model->height / model->rows;
	
	if (model_secondary)
		inner_layers = model->n_layers - DEFAULT_PACK_LAYERS - SEC_PACK_LAYERS;
	else
		inner_layers = model->n_layers - DEFAULT_PACK_LAYERS;

	if (!model->r_ready)
		fatal("R model not ready\n");
	if (!model->has_lcf && flp != model->layers[LAYER_SI].flp)
		fatal("different floorplans for R and C models!\n");

	/* package C's	*/
	populate_package_C(&model->pack, &model->config, model->width, model->height);

	/* layer specific capacitances	*/
	for(i=0; i < model->n_layers; i++)
		model->layers[i].c =  getcap(model->layers[i].sp, model->layers[i].thickness, cw * ch);
		if (model_secondary && (i==inner_layers-SEC_CHIP_LAYERS+LAYER_C4))
			model->layers[i].c1 =  getcap(model->layers[i].sp1, model->layers[i].thickness, cw * ch);
	
	if (!model_secondary) {		
		/* last layer (heatsink) is connected to the ambient. 
	 	* divide c_convec proportional to cell area 
	 	*/
		model->layers[model->n_layers-DEFAULT_PACK_LAYERS+LAYER_SINK].c += C_FACTOR * model->config.c_convec * (cw * ch) /
										  (model->config.s_sink * model->config.s_sink);
	} else {
		model->layers[model->n_layers-DEFAULT_PACK_LAYERS-SEC_PACK_LAYERS+LAYER_SINK].c += C_FACTOR * model->config.c_convec * (cw * ch) /
										  (model->config.s_sink * model->config.s_sink);
		model->layers[model->n_layers-SEC_PACK_LAYERS+LAYER_PCB].c += C_FACTOR * model->config.c_convec_sec * (cw * ch) /
										  (model->config.s_pcb * model->config.s_pcb);
	}

	/* done	*/	
	model->c_ready = TRUE;
}

/* destructor	*/
void delete_grid_model(grid_model_t *model)
{
	int i;
	
	int inner_layers;
	int model_secondary = model->config.model_secondary;
	
	if (model_secondary)
		inner_layers = model->n_layers - DEFAULT_PACK_LAYERS - SEC_PACK_LAYERS;
	else
		inner_layers = model->n_layers - DEFAULT_PACK_LAYERS;

	if (model->has_lcf) 
		for(i=0; i < inner_layers; i++) {
			delete_b2gmap(model->layers[i].b2gmap, model->rows, model->cols);
			free(model->layers[i].g2bmap);
			free_flp(model->layers[i].flp, FALSE);
		}
	/* only the silicon layer has allocated space for the maps. 
	 * all the rest just point to it. also, its floorplan was
	 * allocated elsewhere. so, we don't need to deallocate those.
	 */
	else {
		delete_b2gmap(model->layers[LAYER_SI].b2gmap, model->rows, model->cols);
		free(model->layers[LAYER_SI].g2bmap);
	}
	
	free_grid_model_vector(model->last_steady);
	free_grid_model_vector(model->last_trans);
	free(model->layers);
	free(model);
}

/* differs from 'dvector()' in that memory for internal nodes is also allocated	*/
double *hotspot_vector_grid(grid_model_t *model)
{
	double total_nodes;
	if (model->total_n_blocks <= 0)
		fatal("total_n_blocks is not greater than zero\n");
	
	if (model->config.model_secondary)
		total_nodes = model->total_n_blocks + EXTRA + EXTRA_SEC;
	else
		total_nodes = model->total_n_blocks + EXTRA;
	
	return dvector(total_nodes);
}

/* copy 'src' to 'dst' except for a window of 'size'
 * elements starting at 'at'. useful in floorplan
 * compaction. can be used only with default layer
 * configuration as all layers should have the same
 * floorplan. incompatible with layer configuration
 * file.
 */
void trim_hotspot_vector_grid(grid_model_t *model, double *dst, double *src, 
						 	  int at, int size)
{
	int i;
	
	double total_nodes;
	if (model->config.model_secondary)
		total_nodes = model->total_n_blocks + EXTRA + EXTRA_SEC;
	else
		total_nodes = model->total_n_blocks + EXTRA;
	
	if (model->has_lcf)
		fatal("trim_hotspot_vector_grid called with lcf file\n");
	for (i=0; i < at && i < total_nodes; i++)
		dst[i] = src[i];
	for(i=at+size; i < total_nodes; i++)
		dst[i-size] = src[i];
}

/* update the model corresponding to floorplan compaction	*/						 
void resize_thermal_model_grid(grid_model_t *model, int n_units)
{
	int i;

	if (model->has_lcf)
		fatal("resize_thermal_model_grid called with lcf file\n");
	if (n_units > model->base_n_units)
		fatal("resizing grid model to more than the allocated space\n");

	/* count the total no. of blocks again */
	model->total_n_blocks = 0;
	for(i=0; i < model->n_layers; i++)
		model->total_n_blocks += model->layers[i].flp->n_units;

	/* nothing more needs to be done because the only data structure
	 * that is dependent on flp->n_units is g2bmap (others are
	 * dependent on 'grid size' which does not change because
	 * of resizing). g2bmap is a 1-d array and needs no reallocation
	 */
}

/* sets the temperature of a vector 'temp' allocated using 'hotspot_vector'	*/
void set_temp_grid(grid_model_t *model, double *temp, double val)
{
	int i;
	
	double total_nodes;
	if (model->config.model_secondary)
		total_nodes = model->total_n_blocks + EXTRA + EXTRA_SEC;
	else
		total_nodes = model->total_n_blocks + EXTRA;
		
	if (model->total_n_blocks <= 0)
		fatal("total_n_blocks is not greater than zero\n");
	for(i=0; i < total_nodes; i++)
		temp[i] = val;
}

/* dump the steady state grid temperatures of the top layer onto 'file'	*/
void dump_top_layer_temp_grid (grid_model_t *model, char *file, 
										grid_model_vector_t *temp)
{
	int i, j;
	char str[STR_SIZE];
	FILE *fp;

	if (!model->r_ready)
		fatal("R model not ready\n");

	if (!strcasecmp(file, "stdout"))
		fp = stdout;
	else if (!strcasecmp(file, "stderr"))
		fp = stderr;
	else 	
		fp = fopen (file, "w");

	if (!fp) {
		sprintf (str,"error: %s could not be opened for writing\n", file);
		fatal(str);
	}

	for(i=0;  i < model->rows; i++)
		for(j=0;  j < model->cols; j++)
			fprintf(fp, "%d\t%.2f\n", i*model->cols+j, 
					/* top layer of the most-recently computed 
					 * steady state temperature	
					 */
					model->last_steady->cuboid[0][i][j]); 
		
	if(fp != stdout && fp != stderr)
		fclose(fp);	
}

/* dump the steady state grid temperatures of the top layer onto 'file'	*/
void dump_steady_temp_grid (grid_model_t *model, char *file)
{
	/* top layer of the most-recently computed steady state temperature	*/
	dump_top_layer_temp_grid(model, file, model->last_steady);
}

/* dump temperature vector alloced using 'hotspot_vector' to 'file' */ 
void dump_temp_grid(grid_model_t *model, double *temp, char *file)
{
	int i, n, base = 0;
	char str[STR_SIZE];
	FILE *fp;
	
	int extra_nodes;
	int model_secondary = model->config.model_secondary;
	if (model_secondary)
		extra_nodes = EXTRA + EXTRA_SEC;
	else
		extra_nodes = EXTRA;

	if (!strcasecmp(file, "stdout"))
		fp = stdout;
	else if (!strcasecmp(file, "stderr"))
		fp = stderr;
	else 	
		fp = fopen (file, "w");

	if (!fp) {
		sprintf (str,"error: %s could not be opened for writing\n", file);
		fatal(str);
	}

	/* layer temperatures	*/
	for(n=0; n < model->n_layers; n++) {
		if (!model_secondary) {
			/* default set of layers	*/
			if (!model->has_lcf) {
				switch(n)
				{
					case LAYER_SI:
							strcpy(str,"");
							break;
					case LAYER_INT:
							strcpy(str,"iface_");
							break;
					case DEFAULT_CHIP_LAYERS+LAYER_SP:
							strcpy(str,"hsp_");
							break;
					case DEFAULT_CHIP_LAYERS+LAYER_SINK:
							strcpy(str,"hsink_");
							break;
					default:
							fatal("unknown layer\n");
							break;
				}
			/* layer configuration file	*/
			} else {
					if (n == (model->n_layers - DEFAULT_PACK_LAYERS + LAYER_SP))
						strcpy(str, "hsp_");	/* spreader layer	*/
					else if (n == (model->n_layers - DEFAULT_PACK_LAYERS + LAYER_SINK))
						strcpy(str, "hsink_");	/* heatsink layer	*/
					else	/* other layers	*/
						sprintf(str,"layer_%d_", n);
			}
		} else {
			/* default set of layers	*/
			if (!model->has_lcf) {
				switch(n)
				{
					case LAYER_SI:
							strcpy(str,"");
							break;
					case LAYER_INT:
							strcpy(str,"iface_");
							break;
					case DEFAULT_CHIP_LAYERS+SEC_CHIP_LAYERS+LAYER_SP:
							strcpy(str,"hsp_");
							break;
					case DEFAULT_CHIP_LAYERS+SEC_CHIP_LAYERS+LAYER_SINK:
							strcpy(str,"hsink_");
							break;
					case DEFAULT_CHIP_LAYERS+LAYER_METAL:
							strcpy(str,"metal_");
							break;
					case DEFAULT_CHIP_LAYERS+LAYER_C4:
							strcpy(str,"c4_");
							break;
					case DEFAULT_CHIP_LAYERS+SEC_CHIP_LAYERS+DEFAULT_PACK_LAYERS+LAYER_SUB:
							strcpy(str,"sub_");
							break;
					case DEFAULT_CHIP_LAYERS+SEC_CHIP_LAYERS+DEFAULT_PACK_LAYERS+LAYER_SOLDER:
							strcpy(str,"solder_");
							break;
					case DEFAULT_CHIP_LAYERS+SEC_CHIP_LAYERS+DEFAULT_PACK_LAYERS+LAYER_PCB:
							strcpy(str,"pcb_");
							break;
					default:
							fatal("unknown layer\n");
							break;
				}
			/* layer configuration file	*/
			} else {
					if (n == (model->n_layers - DEFAULT_PACK_LAYERS - SEC_PACK_LAYERS + LAYER_SP))
						strcpy(str, "hsp_");	/* spreader layer	*/
					else if (n == (model->n_layers - DEFAULT_PACK_LAYERS - SEC_PACK_LAYERS + LAYER_SINK))
						strcpy(str, "hsink_");	/* heatsink layer	*/
					else if (n == (model->n_layers - SEC_PACK_LAYERS + LAYER_SUB))
						strcpy(str, "sub_");	/* package substrate layer	*/
					else if (n == (model->n_layers - SEC_PACK_LAYERS + LAYER_SOLDER))
						strcpy(str, "solder_");	/* solder layer	*/
					else if (n == (model->n_layers - SEC_PACK_LAYERS + LAYER_PCB))
						strcpy(str, "pcb_");	/* pcb layer	*/
					else	/* other layers	*/
						sprintf(str,"layer_%d_", n);
			}
		}

		for(i=0; i < model->layers[n].flp->n_units; i++)
			fprintf(fp, "%s%s\t%.2f\n", str, 
					model->layers[n].flp->units[i].name, temp[base+i]);
		base += model->layers[n].flp->n_units;	
	}

	if (base != model->total_n_blocks)
		fatal("total_n_blocks failed to tally\n");

	/* internal node temperatures	*/
	for (i=0; i < extra_nodes; i++) {
			sprintf(str, "inode_%d", i);
			fprintf(fp, "%s\t%.2f\n", str, temp[base+i]);
		}
	if(fp != stdout && fp != stderr)
		fclose(fp);	
}

void copy_temp_grid(grid_model_t *model, double *dst, double *src)
{
	if (!model->config.model_secondary)
		copy_dvector(dst, src, model->total_n_blocks + EXTRA);
	else
		copy_dvector(dst, src, model->total_n_blocks + EXTRA + EXTRA_SEC);
}

/* 
 * read temperature vector alloced using 'hotspot_vector' from 'file'
 * which was dumped using 'dump_temp'. values are clipped to thermal
 * threshold based on 'clip'
 */ 
void read_temp_grid(grid_model_t *model, double *temp, char *file, int clip)
{
	int i, n, idx, base = 0;
	double max=0, val;
	char *ptr, str1[LINE_SIZE], str2[LINE_SIZE];
	char name[STR_SIZE], format[STR_SIZE];
	FILE *fp;
	
	int model_secondary = model->config.model_secondary;
	int extra_nodes;
	if (model->config.model_secondary)
		extra_nodes = EXTRA + EXTRA_SEC;
	else
		extra_nodes = EXTRA;

	if (!strcasecmp(file, "stdin"))
		fp = stdin;
	else
		fp = fopen (file, "r");

	if (!fp) {
		sprintf (str1,"error: %s could not be opened for reading\n", file);
		fatal(str1);
	}	

	/* temperatures of the different layers	*/
	for (n=0; n < model->n_layers; n++) {
		if (!model_secondary) {
			/* default set of layers	*/
			if (!model->has_lcf) {
				switch(n)
				{
					case LAYER_SI:
							strcpy(format,"%s%lf");
							break;
					case LAYER_INT:
							strcpy(format,"iface_%s%lf");
							break;
					case DEFAULT_CHIP_LAYERS+LAYER_SP:
							strcpy(format,"hsp_%s%lf");
							break;
					case DEFAULT_CHIP_LAYERS+LAYER_SINK:
							strcpy(format,"hsink_%s%lf");
							break;
					default:
							fatal("unknown layer\n");
							break;
				}
			/* layer configuration file	*/
			} else {
				if (!model->config.model_secondary) {
					if (n == (model->n_layers - DEFAULT_PACK_LAYERS + LAYER_SP))
						strcpy(format, "hsp_%s%lf");	/* spreader layer	*/
					else if (n == (model->n_layers - DEFAULT_PACK_LAYERS + LAYER_SINK))
						strcpy(format, "hsink_%s%lf");	/* heatsink layer	*/
					else	/* other layers	*/
						sprintf(format,"layer_%d_%%s%%lf", n);
				}
			}
		} else {
			/* default set of layers	*/
			if (!model->has_lcf) {
				switch(n)
				{
					case LAYER_SI:
							strcpy(format,"%s%lf");
							break;
					case LAYER_INT:
							strcpy(format,"iface_%s%lf");
							break;
					case DEFAULT_CHIP_LAYERS+SEC_CHIP_LAYERS+LAYER_SP:
							strcpy(format,"hsp_%s%lf");
							break;
					case DEFAULT_CHIP_LAYERS+SEC_CHIP_LAYERS+LAYER_SINK:
							strcpy(format,"hsink_%s%lf");
							break;
					case DEFAULT_CHIP_LAYERS+LAYER_METAL:
							strcpy(format,"metal_%s%lf");
							break;
					case DEFAULT_CHIP_LAYERS+LAYER_C4:
							strcpy(format,"c4_%s%lf");
							break;
					case DEFAULT_CHIP_LAYERS+SEC_CHIP_LAYERS+DEFAULT_PACK_LAYERS+LAYER_SUB:
							strcpy(format,"sub_%s%lf");
							break;
					case DEFAULT_CHIP_LAYERS+SEC_CHIP_LAYERS+DEFAULT_PACK_LAYERS+LAYER_SOLDER:
							strcpy(format,"solder_%s%lf");
							break;
					case DEFAULT_CHIP_LAYERS+SEC_CHIP_LAYERS+DEFAULT_PACK_LAYERS+LAYER_PCB:
							strcpy(format,"pcb_%s%lf");
							break;
					default:
							fatal("unknown layer\n");
							break;
				}
			/* layer configuration file	*/
			} else {
						if (n == (model->n_layers - DEFAULT_PACK_LAYERS - SEC_PACK_LAYERS + LAYER_SP))
							strcpy(format, "hsp_%s%lf");	/* spreader layer	*/
						else if (n == (model->n_layers - DEFAULT_PACK_LAYERS - SEC_PACK_LAYERS + LAYER_SINK))
							strcpy(format, "hsink_%s%lf");	/* heatsink layer	*/
						else if (n == (model->n_layers - SEC_PACK_LAYERS + LAYER_SUB))
							strcpy(format, "sub_%s%lf");	/* package substrate layer	*/
						else if (n == (model->n_layers - SEC_PACK_LAYERS + LAYER_SOLDER))
							strcpy(format, "solder_%s%lf");	/* solder ball layer	*/
						else if (n == (model->n_layers - SEC_PACK_LAYERS + LAYER_PCB))
							strcpy(format, "pcb_%s%lf");	/* PCB layer	*/
						else	/* other layers	*/
							sprintf(format,"layer_%d_%%s%%lf", n);
			}
		}

		for (i=0; i < model->layers[n].flp->n_units; i++) {
			fgets(str1, LINE_SIZE, fp);
			if (feof(fp))
				fatal("not enough lines in temperature file\n");
			strcpy(str2, str1);
			/* ignore comments and empty lines	*/
			ptr = strtok(str1, " \r\t\n");
			if (!ptr || ptr[0] == '#') {
				i--;
				continue;
			}
			if (sscanf(str2, format, name, &val) != 2)
				fatal("invalid temperature file format\n");
			idx = get_blk_index(model->layers[n].flp, name);
			if (idx >= 0)
				temp[base+idx] = val;
			else	/* since get_blk_index calls fatal, the line below cannot be reached	*/
				fatal ("unit in temperature file not found in floorplan\n");

			/* find max temp on the top layer 
			 * (silicon for the default set of layers)
			 */
			if (n == 0 && temp[idx] > max)
				max = temp[idx];
		}
		base += model->layers[n].flp->n_units;
	}

	if (base != model->total_n_blocks)
		fatal("total_n_blocks failed to tally\n");

	/* internal node temperatures	*/
	for (i=0; i < extra_nodes; i++) {
		fgets(str1, LINE_SIZE, fp);
		if (feof(fp))
			fatal("not enough lines in temperature file\n");
		strcpy(str2, str1);
		/* ignore comments and empty lines	*/
		ptr = strtok(str1, " \r\t\n");
		if (!ptr || ptr[0] == '#') {
			i--;
			continue;
		}
		if (sscanf(str2, "%s%lf", name, &val) != 2)
			fatal("invalid temperature file format\n");
		sprintf(str1, "inode_%d", i);
		if (strcasecmp(str1, name))
			fatal("invalid temperature file format\n");
		temp[base+i] = val;	
	}

	fgets(str1, LINE_SIZE, fp);
	if (!feof(fp))
		fatal("too many lines in temperature file\n");

	if(fp != stdin)
		fclose(fp);	

	/* clipping	*/
	if (clip && (max > model->config.thermal_threshold)) {
		/* if max has to be brought down to thermal_threshold, 
		 * (w.r.t the ambient) what is the scale down factor?
		 */
		double factor = (model->config.thermal_threshold - model->config.ambient) / 
						(max - model->config.ambient);
	
		/* scale down all temperature differences (from ambient) by the same factor	*/
		for (i=0; i < model->total_n_blocks + extra_nodes; i++)
			temp[i] = (temp[i]-model->config.ambient)*factor + model->config.ambient;
	}
}

/* dump power numbers to file	*/
void dump_power_grid(grid_model_t *model, double *power, char *file)
{
	int i, n, base = 0;
	char str[STR_SIZE];
	FILE *fp;

	if (!strcasecmp(file, "stdout"))
		fp = stdout;
	else if (!strcasecmp(file, "stderr"))
		fp = stderr;
	else 	
		fp = fopen (file, "w");
	if (!fp) {
		sprintf (str,"error: %s could not be opened for writing\n", file);
		fatal(str);
	}

	/* dump values only for the layers dissipating power	*/
	for(n=0; n < model->n_layers; n++) {
		if (model->layers[n].has_power) {
			for(i=0; i < model->layers[n].flp->n_units; i++)
				if (model->has_lcf)
					fprintf(fp, "layer_%d_%s\t%.6f\n", n, 
							model->layers[n].flp->units[i].name, power[base+i]);
				else 
					fprintf(fp, "%s\t%.6f\n", 
							model->layers[n].flp->units[i].name, power[base+i]);
		}				
		base += model->layers[n].flp->n_units;		
	}

	if(fp != stdout && fp != stderr)
		fclose(fp);	
}

/* 
 * read power vector alloced using 'hotspot_vector' from 'file'
 * which was dumped using 'dump_power'. 
 */ 
void read_power_grid (grid_model_t *model, double *power, char *file)
{
	int i, idx, n, base = 0;
	double val;
	char *ptr, str1[LINE_SIZE], str2[LINE_SIZE]; 
	char name[STR_SIZE], format[STR_SIZE];
	FILE *fp;

	if (!strcasecmp(file, "stdin"))
		fp = stdin;
	else
		fp = fopen (file, "r");
	if (!fp) {
		sprintf (str1,"error: %s could not be opened for reading\n", file);
		fatal(str1);
	}

	/* lcf file could potentially specify more than one power dissipating 
	 * layer. hence, units with zero power within a layer cannot be left
	 * out in the power file.
	 */
	if (model->has_lcf) {
		for(n=0; n < model->n_layers; n++) {
			if (model->layers[n].has_power)
				for(i=0; i < model->layers[n].flp->n_units; i++) {
					fgets(str1, LINE_SIZE, fp);
					if (feof(fp))
						fatal("not enough lines in power file\n");
					strcpy(str2, str1);

					/* ignore comments and empty lines	*/
					ptr = strtok(str1, " \r\t\n");
					if (!ptr || ptr[0] == '#') {
						i--;
						continue;
					}

					sprintf(format, "layer_%d_%%s%%lf", n);
		  			if (sscanf(str2, format, name, &val) != 2)
						fatal("invalid power file format\n");
		  			idx = get_blk_index(model->layers[n].flp, name);
		  			if (idx >= 0)
						power[base+idx] = val;
					/* since get_blk_index calls fatal, the line below cannot be reached	*/
		  			else
						fatal ("unit in power file not found in floorplan\n");
				}
			base += model->layers[n].flp->n_units;	
		}
		fgets(str1, LINE_SIZE, fp);
		if (!feof(fp))
			fatal("too many lines in power file\n");
	/* default layer configuration. so only one layer
	 * has power dissipation. units with zero power 
	 * can be omitted in the power file
	 */
	} else {
		while(!feof(fp)) {
			fgets(str1, LINE_SIZE, fp);
			if (feof(fp))
				break;
			strcpy(str2, str1);
	
			/* ignore comments and empty lines	*/
			ptr = strtok(str1, " \r\t\n");
			if (!ptr || ptr[0] == '#')
				continue;
	
			if (sscanf(str2, "%s%lf", name, &val) != 2)
				fatal("invalid power file format\n");
			idx = get_blk_index(model->layers[LAYER_SI].flp, name);
			if (idx >= 0)
				power[idx] = val;
			else	/* since get_blk_index calls fatal, the line below cannot be reached	*/
				fatal ("unit in power file not found in floorplan\n");
		}
	}

	if(fp != stdin)
		fclose(fp);
}

double find_max_temp_grid(grid_model_t *model, double *temp)
{
	int i;
	double max = 0.0;
	/* max temperature occurs on the top-most layer	*/
	for(i=0; i < model->layers[0].flp->n_units; i++) {
		if (temp[i] < 0)
			fatal("negative temperature!\n");
		else if (max < temp[i])
			max = temp[i];
	}

	return max;
}

double find_max_idx_temp_grid(grid_model_t *model, double *temp, int *idx)
{
	int i;
	double max = 0.0;
	/* max temperature occurs on the top-most layer	*/
	for(i=0; i < model->layers[0].flp->n_units; i++) {
		if (temp[i] < 0)
			fatal("negative temperature!\n");
		else if (max < temp[i]){
			max = temp[i];
      (*idx) = i;
    }
	}

	return max;
}
double find_avg_temp_grid(grid_model_t *model, double *temp)
{
	int i, n, base = 0, count = 0; 
	double sum = 0.0;
	/* average temperature of all the power dissipating blocks	*/
	for(n=0; n < model->n_layers; n++) {
		if (model->layers[n].has_power) {
			for(i=0; i < model->layers[n].flp->n_units; i++) {
				if (temp[base+i] < 0)
					fatal("negative temperature!\n");
				else 
					sum += temp[base+i];
			}
			count += model->layers[n].flp->n_units;
		}	
		base += model->layers[n].flp->n_units;
	}

	if (!count)
		fatal("no power dissipating units?!\n");
	return (sum / count);
}

/* calculate average heatsink temperature for natural convection package */
double calc_sink_temp_grid(grid_model_t *model, double *temp, thermal_config_t *config)
{
	int i, n, base = 0; 
	int hsidx = model->n_layers - DEFAULT_PACK_LAYERS + LAYER_SINK;
	double sum = 0.0;
	double spr_size = config->s_spreader*config->s_spreader;
	double sink_size = config->s_sink*config->s_sink;

	/* heat sink core	*/
	for(n=0; n < hsidx; n++)
		base += model->layers[n].flp->n_units;

	for(i=base; i < base+model->layers[n].flp->n_units; i++)
		if (temp[i] < 0)
			fatal("negative temperature!\n");
		else /* area-weighted average */
			sum += temp[i]*(model->layers[n].flp->units[i-base].width*model->layers[n].flp->units[i-base].height);

	/* heat sink periphery	*/
	base = model->total_n_blocks;
	
	for(i=SINK_C_W; i <= SINK_C_E; i++)
		if (temp[i+base] < 0)
			fatal("negative temperature!\n");
		else
			sum += temp[i+base]*0.25*(config->s_spreader+model->height)*(config->s_spreader-model->width);

	for(i=SINK_C_N; i <= SINK_C_S; i++)
		if (temp[i+base] < 0)
			fatal("negative temperature!\n");
		else
			sum += temp[i+base]*0.25*(config->s_spreader+model->width)*(config->s_spreader-model->height);

	for(i=SINK_W; i <= SINK_S; i++)
		if (temp[i+base] < 0)
			fatal("negative temperature!\n");
		else
			sum += temp[i+base]*0.25*(sink_size-spr_size);

	return (sum / sink_size);
}

/* grid_model_vector routines	*/

/* constructor	*/
grid_model_vector_t *new_grid_model_vector(grid_model_t *model)
{
	grid_model_vector_t *v;
	
	int extra_nodes;
	if (model->config.model_secondary)
		extra_nodes = EXTRA + EXTRA_SEC;
	else
		extra_nodes = EXTRA;

	v = (grid_model_vector_t *) calloc (1, sizeof(grid_model_vector_t));
	if (!v)
		fatal("memory allocation error\n");

	v->cuboid = dcuboid_tail(model->rows, model->cols, model->n_layers, extra_nodes);
	v->extra = v->cuboid[0][0] + model->rows * model->cols * model->n_layers;
	return v;
}

/* destructor	*/
void free_grid_model_vector(grid_model_vector_t *v)
{
	free_dcuboid(v->cuboid);
	free(v);
}

/* translate power/temperature between block and grid vectors	*/
void xlate_vector_b2g(grid_model_t *model, double *b, grid_model_vector_t *g, int type)
{
	int i, j, n, base = 0;
	double area;
	
	int extra_nodes;
	if (model->config.model_secondary)
		extra_nodes = EXTRA + EXTRA_SEC;
	else
		extra_nodes = EXTRA;

	/* area of a single grid cell	*/
	area = (model->width * model->height) / (model->cols * model->rows);

	for(n=0; n < model->n_layers; n++) {
		for(i=0; i < model->rows; i++)
			for(j=0; j < model->cols; j++) {
				/* for each grid cell, the power density / temperature are 
				 * the average of the power densities / temperatures of the 
				 * blocks in it weighted by their occupancies
				 */
				/* convert power density to power	*/ 
				if (type == V_POWER)
					g->cuboid[n][i][j] = blist_avg(model->layers[n].b2gmap[i][j], 
										 model->layers[n].flp, &b[base], type) * area;
				/* no conversion necessary for temperature	*/ 
				else if (type == V_TEMP)
					g->cuboid[n][i][j] = blist_avg(model->layers[n].b2gmap[i][j], 
										 model->layers[n].flp, &b[base], type);
				else
					fatal("unknown vector type\n");
			}
		/* keep track of the beginning address of this layer in the 
		 * block power vector
		 */
		base += model->layers[n].flp->n_units;							 
	}

	/* extra spreader and sink nodes	*/
	for(i=0; i < extra_nodes; i++)
		g->extra[i] = b[base+i];
}

/* translate temperature between grid and block vectors	*/
void xlate_temp_g2b(grid_model_t *model, double *b, grid_model_vector_t *g)
{
	int i, j, n, u, base = 0, count;
	int i1, j1, i2, j2, ci1, cj1, ci2, cj2;
	double min, max, avg;
	
	int extra_nodes;
	if (model->config.model_secondary)
		extra_nodes = EXTRA + EXTRA_SEC;
	else
		extra_nodes = EXTRA;

	for(n=0; n < model->n_layers; n++) {
		for(u=0; u < model->layers[n].flp->n_units; u++) {
			/* extent of this unit in grid cell units	*/
			i1 = model->layers[n].g2bmap[u].i1;
			j1 = model->layers[n].g2bmap[u].j1;
			i2 = model->layers[n].g2bmap[u].i2;
			j2 = model->layers[n].g2bmap[u].j2;

			/* map the center grid cell's temperature to the block	*/
			if (model->map_mode == GRID_CENTER) {
				/* center co-ordinates	*/	
				ci1 = (i1 + i2) / 2;
				cj1 = (j1 + j2) / 2;
				/* in case of even no. of cells, center 
				 * is the average of two central cells
				 */
				/* ci2 = ci1-1 when even, ci1 otherwise	*/  
				ci2 = ci1 - !((i2-i1) % 2);
				/* cj2 = cj1-1 when even, cj1 otherwise	*/  
				cj2 = cj1 - !((j2-j1) % 2);

				b[base+u] = (g->cuboid[n][ci1][cj1] + g->cuboid[n][ci2][cj1] + 
						    g->cuboid[n][ci1][cj2] + g->cuboid[n][ci2][cj2]) / 4;
				continue;
			}
		
			/* find the min/max/avg temperatures of the 
			 * grid cells in this block
			 */
			avg = 0.0;
			count = 0;
			min = max = g->cuboid[n][i1][j1];
			for(i=i1; i < i2; i++)
				for(j=j1; j < j2; j++) {
					avg += g->cuboid[n][i][j];
					if (g->cuboid[n][i][j] < min)
						min = g->cuboid[n][i][j];
					if (g->cuboid[n][i][j] > max)
						max = g->cuboid[n][i][j];
					count++;
				}

			/* map to output accordingly	*/
			switch (model->map_mode)
			{
				case GRID_AVG:
					b[base+u] = avg / count;
					break;
				case GRID_MIN:
					b[base+u] = min;
					break;
				case GRID_MAX:
					b[base+u] = max;
					break;
				/* taken care of already	*/	
				case GRID_CENTER:
					break;
				default:
					fatal("unknown mapping mode\n");
					break;
			}
		}
		/* keep track of the beginning address of this layer in the 
		 * block power vector
		 */
		base += model->layers[n].flp->n_units;							 
	}

	/* extra spreader and sink nodes	*/
	for(i=0; i < extra_nodes; i++)
		b[base+i] = g->extra[i];
}

/* setting package nodes' power numbers	*/
void set_internal_power_grid(grid_model_t *model, double *power)
{
	if (!model->config.model_secondary)
		zero_dvector(&power[model->total_n_blocks], EXTRA);
	else
		zero_dvector(&power[model->total_n_blocks], EXTRA+EXTRA_SEC);
}

/* set up initial temperatures for the steady state solution
 * heuristically (ignoring the lateral resistances)
 */
void set_heuristic_temp(grid_model_t *model, grid_model_vector_t *power, 
						grid_model_vector_t *temp)
{
	int n, i, j, nl, nr, nc;
	double **sum;

	/* shortcuts	*/
	nl = model->n_layers;
	nr = model->rows;
	nc = model->cols;

	/* package temperatures	*/
	/* if all lateral resistances are considered infinity, all peripheral 
	 * package nodes are at the ambient temperature
	 */
	temp->extra[SINK_N] = temp->extra[SINK_S] = 
						  temp->extra[SINK_E] = temp->extra[SINK_W] = 
						  temp->extra[SINK_C_N] = temp->extra[SINK_C_S] = 
						  temp->extra[SINK_C_E] = temp->extra[SINK_C_W] = 
						  temp->extra[SP_N] = temp->extra[SP_S] = 
						  temp->extra[SP_E] = temp->extra[SP_W] =
						  model->config.ambient;
	
	if (model->config.model_secondary) {
		temp->extra[PCB_N] = temp->extra[PCB_S] = 
						  temp->extra[PCB_E] = temp->extra[PCB_W] = 
						  temp->extra[PCB_C_N] = temp->extra[PCB_C_S] = 
						  temp->extra[PCB_C_E] = temp->extra[PCB_C_W] =
						  temp->extra[SOLDER_N] = temp->extra[SOLDER_S] =
						  temp->extra[SOLDER_E] = temp->extra[SOLDER_W] = 
						  temp->extra[SUB_N] = temp->extra[SUB_S] = 
						  temp->extra[SUB_E] = temp->extra[SUB_W] =
						  model->config.ambient;
	}

	/* layer temperatures	*/
	/* add up power for each grid cell across all layers */
	sum = dmatrix(nr, nc);
	for(n=0; n < nl; n++)
		scaleadd_dvector(sum[0], sum[0], power->cuboid[n][0], nr*nc, 1.0);
	
	/* last layer	*/
		for(i=0; i < nr; i++)
			for(j=0; j < nc; j++)
				temp->cuboid[nl-1][i][j] = model->config.ambient + sum[i][j] * 
										 model->layers[nl-1].rz;
		/* subtract away the layer's power	*/			
		scaleadd_dvector(sum[0], sum[0], power->cuboid[nl-1][0], nr*nc, -1.0);
				
		/* go from last-1 to first	*/
		for(n=nl-2; n >= 0; n--) {
			/* nth layer temp is n+1th temp + cumul_power * rz of the nth layer	*/
			scaleadd_dvector(temp->cuboid[n][0], temp->cuboid[n+1][0], sum[0], 
							 nr*nc, model->layers[n].rz);
			/* subtract away the layer's power	*/			
			scaleadd_dvector(sum[0], sum[0], power->cuboid[n][0], nr*nc, -1.0);
		} 
	free_dmatrix(sum); 
}

/* single steady state iteration of grid solver - package part */
double single_iteration_steady_pack(grid_model_t *model, grid_model_vector_t *power,
									grid_model_vector_t *temp)
{
	int i, j;
//	int extra_nodes;
//	if (model->config.model_secondary) 
//		extra_nodes = EXTRA + EXTRA_SEC;
//	else 
//		extra_nodes = EXTRA;
		
	double delta[EXTRA+EXTRA_SEC], max = 0;
	/* sum of the conductances	*/
	double csum;
	/* weighted sum of temperatures	*/
	double wsum;

	/* shortcuts	*/
	double *v = temp->extra;
	package_RC_t *pk = &model->pack;
	thermal_config_t *c = &model->config;
	layer_t *l = model->layers;
	int nl = model->n_layers;
	int nr = model->rows;
	int nc = model->cols;
	int spidx, hsidx, subidx, solderidx, pcbidx;
	
	if (!model->config.model_secondary) {
		spidx = nl - DEFAULT_PACK_LAYERS + LAYER_SP;
		hsidx = nl - DEFAULT_PACK_LAYERS + LAYER_SINK;
	} else {
		spidx = nl - DEFAULT_PACK_LAYERS - SEC_PACK_LAYERS + LAYER_SP;
		hsidx = nl - DEFAULT_PACK_LAYERS - SEC_PACK_LAYERS + LAYER_SINK;
		subidx = nl - SEC_PACK_LAYERS + LAYER_SUB;
		solderidx = nl - SEC_PACK_LAYERS + LAYER_SOLDER;
		pcbidx = nl -SEC_PACK_LAYERS + LAYER_PCB;	
		
	}

	/* sink outer north/south	*/
	csum = 1.0/(pk->r_hs_per + pk->r_amb_per) + 1.0/(pk->r_hs2_y + pk->r_hs);
	wsum = c->ambient/(pk->r_hs_per + pk->r_amb_per) + v[SINK_C_N]/(pk->r_hs2_y + pk->r_hs);
	delta[SINK_N] = fabs(v[SINK_N] - wsum / csum);
	v[SINK_N] = wsum / csum;
	wsum = c->ambient/(pk->r_hs_per + pk->r_amb_per) + v[SINK_C_S]/(pk->r_hs2_y + pk->r_hs);
	delta[SINK_S] = fabs(v[SINK_S] - wsum / csum);
	v[SINK_S] = wsum / csum;
	
	/* sink outer west/east	*/
	csum = 1.0/(pk->r_hs_per + pk->r_amb_per) + 1.0/(pk->r_hs2_x + pk->r_hs);
	wsum = c->ambient/(pk->r_hs_per + pk->r_amb_per) + v[SINK_C_W]/(pk->r_hs2_x + pk->r_hs);
	delta[SINK_W] = fabs(v[SINK_W] - wsum / csum);
	v[SINK_W] = wsum / csum;
	wsum = c->ambient/(pk->r_hs_per + pk->r_amb_per) + v[SINK_C_E]/(pk->r_hs2_x + pk->r_hs);
	delta[SINK_E] = fabs(v[SINK_E] - wsum / csum);
	v[SINK_E] = wsum / csum;

	/* sink inner north/south	*/
	/* partition r_hs1_y among all the nc grid cells. edge cell has half the ry */
	csum = nc / (l[hsidx].ry / 2.0 + nc * pk->r_hs1_y);
	csum += 1.0/(pk->r_hs_c_per_y + pk->r_amb_c_per_y) + 
			1.0/pk->r_sp_per_y + 1.0/(pk->r_hs2_y + pk->r_hs);

	wsum = 0.0;
	for(j=0; j < nc; j++)
		wsum += temp->cuboid[hsidx][0][j];
	wsum /= (l[hsidx].ry / 2.0 + nc * pk->r_hs1_y);
	wsum += c->ambient/(pk->r_hs_c_per_y + pk->r_amb_c_per_y) + 
			v[SP_N]/pk->r_sp_per_y + v[SINK_N]/(pk->r_hs2_y + pk->r_hs);
	delta[SINK_C_N] = fabs(v[SINK_C_N] - wsum / csum);
	v[SINK_C_N] = wsum / csum;

	wsum = 0.0;
	for(j=0; j < nc; j++)
		wsum += temp->cuboid[hsidx][nr-1][j];
	wsum /= (l[hsidx].ry / 2.0 + nc * pk->r_hs1_y);
	wsum += c->ambient/(pk->r_hs_c_per_y + pk->r_amb_c_per_y) + 
			v[SP_S]/pk->r_sp_per_y + v[SINK_S]/(pk->r_hs2_y + pk->r_hs);
	delta[SINK_C_S] = fabs(v[SINK_C_S] - wsum / csum);
	v[SINK_C_S] = wsum / csum;

	/* sink inner west/east	*/
	/* partition r_hs1_x among all the nr grid cells. edge cell has half the rx */
	csum = nr / (l[hsidx].rx / 2.0 + nr * pk->r_hs1_x);
	csum += 1.0/(pk->r_hs_c_per_x + pk->r_amb_c_per_x) + 
			1.0/pk->r_sp_per_x + 1.0/(pk->r_hs2_x + pk->r_hs);

	wsum = 0.0;
	for(i=0; i < nr; i++)
		wsum += temp->cuboid[hsidx][i][0];
	wsum /= (l[hsidx].rx / 2.0 + nr * pk->r_hs1_x);
	wsum += c->ambient/(pk->r_hs_c_per_x + pk->r_amb_c_per_x) + 
			v[SP_W]/pk->r_sp_per_x + v[SINK_W]/(pk->r_hs2_x + pk->r_hs);
	delta[SINK_C_W] = fabs(v[SINK_C_W] - wsum / csum);
	v[SINK_C_W] = wsum / csum;

	wsum = 0.0;
	for(i=0; i < nr; i++)
		wsum += temp->cuboid[hsidx][i][nc-1];
	wsum /= (l[hsidx].rx / 2.0 + nr * pk->r_hs1_x);
	wsum += c->ambient/(pk->r_hs_c_per_x + pk->r_amb_c_per_x) + 
			v[SP_E]/pk->r_sp_per_x + v[SINK_E]/(pk->r_hs2_x + pk->r_hs);
	delta[SINK_C_E] = fabs(v[SINK_C_E] - wsum / csum);
	v[SINK_C_E] = wsum / csum;

	/* spreader north/south	*/
	/* partition r_sp1_y among all the nc grid cells. edge cell has half the ry */
	csum = nc / (l[spidx].ry / 2.0 + nc * pk->r_sp1_y);
	csum += 1.0/pk->r_sp_per_y;

	wsum = 0.0;
	for(j=0; j < nc; j++)
		wsum += temp->cuboid[spidx][0][j];
	wsum /= (l[spidx].ry / 2.0 + nc * pk->r_sp1_y);
	wsum += v[SINK_C_N]/pk->r_sp_per_y;
	delta[SP_N] = fabs(v[SP_N] - wsum / csum);
	v[SP_N] = wsum / csum;

	wsum = 0.0;
	for(j=0; j < nc; j++)
		wsum += temp->cuboid[spidx][nr-1][j];
	wsum /= (l[spidx].ry / 2.0 + nc * pk->r_sp1_y);
	wsum += v[SINK_C_S]/pk->r_sp_per_y;
	delta[SP_S] = fabs(v[SP_S] - wsum / csum);
	v[SP_S] = wsum / csum;

	/* spreader west/east	*/
	/* partition r_sp1_x among all the nr grid cells. edge cell has half the rx */
	csum = nr / (l[spidx].rx / 2.0 + nr * pk->r_sp1_x);
	csum += 1.0/pk->r_sp_per_x;

	wsum = 0.0;
	for(i=0; i < nr; i++)
		wsum += temp->cuboid[spidx][i][0];
	wsum /= (l[spidx].rx / 2.0 + nr * pk->r_sp1_x);
	wsum += v[SINK_C_W]/pk->r_sp_per_x;
	delta[SP_W] = fabs(v[SP_W] - wsum / csum);
	v[SP_W] = wsum / csum;

	wsum = 0.0;
	for(i=0; i < nr; i++)
		wsum += temp->cuboid[spidx][i][nc-1];
	wsum /= (l[spidx].rx / 2.0 + nr * pk->r_sp1_x);
	wsum += v[SINK_C_E]/pk->r_sp_per_x;
	delta[SP_E] = fabs(v[SP_E] - wsum / csum);
	v[SP_E] = wsum / csum;
	
	if (model->config.model_secondary) {
		/* secondary path package nodes */
		/* PCB outer north/south	*/
		csum = 1.0/(pk->r_amb_sec_per) + 1.0/(pk->r_pcb2_y + pk->r_pcb);
		wsum = c->ambient/(pk->r_amb_sec_per) + v[PCB_C_N]/(pk->r_pcb2_y + pk->r_pcb);
		delta[PCB_N] = fabs(v[PCB_N] - wsum / csum);
		v[PCB_N] = wsum / csum;
		wsum = c->ambient/(pk->r_amb_sec_per) + v[PCB_C_S]/(pk->r_pcb2_y + pk->r_pcb);
		delta[PCB_S] = fabs(v[PCB_S] - wsum / csum);
		v[PCB_S] = wsum / csum;
		
		/* PCB outer west/east	*/
		csum = 1.0/(pk->r_amb_sec_per) + 1.0/(pk->r_pcb2_x + pk->r_pcb);
		wsum = c->ambient/(pk->r_amb_sec_per) + v[PCB_C_W]/(pk->r_pcb2_x + pk->r_pcb);
		delta[PCB_W] = fabs(v[PCB_W] - wsum / csum);
		v[PCB_W] = wsum / csum;
		wsum = c->ambient/(pk->r_amb_sec_per) + v[PCB_C_E]/(pk->r_pcb2_x + pk->r_pcb);
		delta[PCB_E] = fabs(v[PCB_E] - wsum / csum);
		v[PCB_E] = wsum / csum;
  	
		/* PCB inner north/south	*/
		/* partition r_pcb1_y among all the nc grid cells. edge cell has half the ry */
		csum = nc / (l[pcbidx].ry / 2.0 + nc * pk->r_pcb1_y);
		csum += 1.0/(pk->r_amb_sec_c_per_y) + 
				1.0/pk->r_pcb_c_per_y + 1.0/(pk->r_pcb2_y + pk->r_pcb);
  	
		wsum = 0.0;
		for(j=0; j < nc; j++)
			wsum += temp->cuboid[pcbidx][0][j];
		wsum /= (l[pcbidx].ry / 2.0 + nc * pk->r_pcb1_y);
		wsum += c->ambient/(pk->r_amb_sec_c_per_y) + 
				v[SOLDER_N]/pk->r_pcb_c_per_y + v[PCB_N]/(pk->r_pcb2_y + pk->r_pcb);
		delta[PCB_C_N] = fabs(v[PCB_C_N] - wsum / csum);
		v[PCB_C_N] = wsum / csum;
  	
 		wsum = 0.0;
		for(j=0; j < nc; j++)
			wsum += temp->cuboid[pcbidx][nr-1][j];
		wsum /= (l[pcbidx].ry / 2.0 + nc * pk->r_pcb1_y);
		wsum += c->ambient/(pk->r_amb_sec_c_per_y) + 
				v[SOLDER_S]/pk->r_pcb_c_per_y + v[PCB_S]/(pk->r_pcb2_y + pk->r_pcb);
		delta[PCB_C_S] = fabs(v[PCB_C_S] - wsum / csum);
		v[PCB_C_S] = wsum / csum;
  	
		/* PCB inner west/east	*/
		/* partition r_pcb1_x among all the nr grid cells. edge cell has half the rx */
		csum = nr / (l[pcbidx].rx / 2.0 + nr * pk->r_pcb1_x);
		csum += 1.0/(pk->r_amb_sec_c_per_x) + 
				1.0/pk->r_pcb_c_per_x + 1.0/(pk->r_pcb2_x + pk->r_pcb);
  	
		wsum = 0.0;
		for(i=0; i < nr; i++)
			wsum += temp->cuboid[pcbidx][i][0];
		wsum /= (l[pcbidx].rx / 2.0 + nr * pk->r_pcb1_x);
		wsum += c->ambient/(pk->r_amb_sec_c_per_x) + 
				v[SOLDER_W]/pk->r_pcb_c_per_x + v[PCB_W]/(pk->r_pcb2_x + pk->r_pcb);
		delta[PCB_C_W] = fabs(v[PCB_C_W] - wsum / csum);
		v[PCB_C_W] = wsum / csum;
  	
		wsum = 0.0;
		for(i=0; i < nr; i++)
			wsum += temp->cuboid[pcbidx][i][nc-1];
		wsum /= (l[pcbidx].rx / 2.0 + nr * pk->r_pcb1_x);
		wsum += c->ambient/(pk->r_amb_sec_c_per_x) + 
				v[SOLDER_E]/pk->r_pcb_c_per_x + v[PCB_E]/(pk->r_pcb2_x + pk->r_pcb);
		delta[PCB_C_E] = fabs(v[PCB_C_E] - wsum / csum);
		v[PCB_C_E] = wsum / csum;
  	
		/* solder north/south	*/
		/* partition r_solder1_y among all the nc grid cells. edge cell has half the ry */
		csum = nc / (l[solderidx].ry / 2.0 + nc * pk->r_solder1_y);
		csum += 1.0/pk->r_solder_per_y + 1.0/pk->r_pcb_c_per_y;
  	
		wsum = 0.0;
		for(j=0; j < nc; j++)
			wsum += temp->cuboid[solderidx][0][j];
		wsum /= (l[solderidx].ry / 2.0 + nc * pk->r_solder1_y);
		wsum += v[PCB_C_N]/pk->r_pcb_c_per_y + v[SUB_N]/pk->r_solder_per_y;
		delta[SOLDER_N] = fabs(v[SOLDER_N] - wsum / csum);
		v[SOLDER_N] = wsum / csum;
  	
		wsum = 0.0;
		for(j=0; j < nc; j++)
			wsum += temp->cuboid[solderidx][nr-1][j];
		wsum /= (l[solderidx].ry / 2.0 + nc * pk->r_solder1_y);
		wsum += v[PCB_C_S]/pk->r_pcb_c_per_y + v[SUB_S]/pk->r_solder_per_y;
		delta[SOLDER_S] = fabs(v[SOLDER_S] - wsum / csum);
		v[SOLDER_S] = wsum / csum;
  	
		/* solder west/east	*/
		/* partition r_solder1_x among all the nr grid cells. edge cell has half the rx */
		csum = nr / (l[solderidx].rx / 2.0 + nr * pk->r_solder1_x);
		csum += 1.0/pk->r_solder_per_x + 1.0/pk->r_pcb_c_per_x;
  	
		wsum = 0.0;
		for(i=0; i < nr; i++)
			wsum += temp->cuboid[solderidx][i][0];
		wsum /= (l[solderidx].rx / 2.0 + nr * pk->r_solder1_x);
		wsum += v[PCB_C_W]/pk->r_pcb_c_per_x + v[SUB_W]/pk->r_solder_per_x;
		delta[SOLDER_W] = fabs(v[SOLDER_W] - wsum / csum);
		v[SOLDER_W] = wsum / csum;
  	
		wsum = 0.0;
		for(i=0; i < nr; i++)
			wsum += temp->cuboid[solderidx][i][nc-1];
		wsum /= (l[solderidx].rx / 2.0 + nr * pk->r_solder1_x);
		wsum += v[PCB_C_E]/pk->r_pcb_c_per_x + v[SUB_E]/pk->r_solder_per_x;
		delta[SOLDER_E] = fabs(v[SOLDER_E] - wsum / csum);
		v[SOLDER_E] = wsum / csum; 
		
		/* substrate north/south	*/
		/* partition r_sub1_y among all the nc grid cells. edge cell has half the ry */
		csum = nc / (l[subidx].ry / 2.0 + nc * pk->r_sub1_y);
		csum += 1.0/pk->r_solder_per_y;
  	
		wsum = 0.0;
		for(j=0; j < nc; j++)
			wsum += temp->cuboid[subidx][0][j];
		wsum /= (l[subidx].ry / 2.0 + nc * pk->r_sub1_y);
		wsum += v[SOLDER_N]/pk->r_solder_per_y;
		delta[SUB_N] = fabs(v[SUB_N] - wsum / csum);
		v[SUB_N] = wsum / csum;
		
		wsum = 0.0;
		for(j=0; j < nc; j++)
			wsum += temp->cuboid[subidx][nr-1][j];
		wsum /= (l[subidx].ry / 2.0 + nc * pk->r_sub1_y);
		wsum += v[SOLDER_S]/pk->r_solder_per_y;
		delta[SUB_S] = fabs(v[SUB_S] - wsum / csum);
		v[SUB_S] = wsum / csum;
  	
		/* substrate west/east	*/
		/* partition r_sub1_x among all the nr grid cells. edge cell has half the rx */
		csum = nr / (l[subidx].rx / 2.0 + nr * pk->r_sub1_x);
		csum += 1.0/pk->r_solder_per_x;
  	
		wsum = 0.0;
		for(i=0; i < nr; i++)
			wsum += temp->cuboid[subidx][i][0];
		wsum /= (l[subidx].rx / 2.0 + nr * pk->r_sub1_x);
		wsum += v[SOLDER_W]/pk->r_solder_per_x;
		delta[SUB_W] = fabs(v[SUB_W] - wsum / csum);
		v[SUB_W] = wsum / csum;
  	
		wsum = 0.0;
		for(i=0; i < nr; i++)
			wsum += temp->cuboid[subidx][i][nc-1];
		wsum /= (l[subidx].rx / 2.0 + nr * pk->r_sub1_x);
		wsum += v[SOLDER_E]/pk->r_solder_per_x;
		delta[SUB_E] = fabs(v[SUB_E] - wsum / csum);
		v[SUB_E] = wsum / csum;
	} 
	
	if (!model->config.model_secondary) {
		for(i=0; i < EXTRA; i++) {
			if (delta[i] > max)
				max = delta[i];
		}
	} else {
		for(i=0; i < EXTRA + EXTRA_SEC; i++) {
			if (delta[i] > max)
				max = delta[i];
		}
	}
	return max;
}

/* macros for calculating conductances	*/
/* conductance to the next cell north. zero if on northern boundary	*/
# define NC(l,n,i,j,nl,nr,nc)		((i > 0) ? (1.0/l[n].ry) : 0.0)
/* conductance to the next cell south. zero if on southern boundary	*/
# define SC(l,n,i,j,nl,nr,nc)		((i < nr-1) ? (1.0/l[n].ry) : 0.0)
/* conductance to the next cell east. zero if on eastern boundary	*/
# define EC(l,n,i,j,nl,nr,nc)		((j < nc-1) ? (1.0/l[n].rx) : 0.0)
/* conductance to the next cell west. zero if on western boundary	*/
# define WC(l,n,i,j,nl,nr,nc)		((j > 0) ? (1.0/l[n].rx) : 0.0)
/* conductance to the next cell below. zero if on bottom face		*/
# define BC(l,n,i,j,nl,nr,nc)		((n < nl-1) ? (1.0/l[n].rz) : 0.0)
/* conductance to the next cell above. zero if on top face			*/
# define AC(l,n,i,j,nl,nr,nc)		((n > 0) ? (1.0/l[n-1].rz) : 0.0)

/* macros for calculating weighted temperatures	*/
/* weighted T of the next cell north. zero if on northern boundary	*/
# define NT(l,v,n,i,j,nl,nr,nc)		((i > 0) ? (v[n][i-1][j]/l[n].ry) : 0.0)
/* weighted T of the next cell south. zero if on southern boundary	*/
# define ST(l,v,n,i,j,nl,nr,nc)		((i < nr-1) ? (v[n][i+1][j]/l[n].ry) : 0.0)
/* weighted T of the next cell east. zero if on eastern boundary	*/
# define ET(l,v,n,i,j,nl,nr,nc)		((j < nc-1) ? (v[n][i][j+1]/l[n].rx) : 0.0)
/* weighted T of the next cell west. zero if on western boundary	*/
# define WT(l,v,n,i,j,nl,nr,nc)		((j > 0) ? (v[n][i][j-1]/l[n].rx) : 0.0)
/* weighted T of the next cell below. zero if on bottom face		*/
# define BT(l,v,n,i,j,nl,nr,nc)		((n < nl-1) ? (v[n+1][i][j]/l[n].rz) : 0.0)
/* weighted T of the next cell above. zero if on top face			*/
# define AT(l,v,n,i,j,nl,nr,nc)		((n > 0) ? (v[n-1][i][j]/l[n-1].rz) : 0.0)


/* single steady state iteration of grid solver - silicon part */
double single_iteration_steady_grid(grid_model_t *model, grid_model_vector_t *power,
									grid_model_vector_t *temp)
{
	int n, i, j;
	double prev, delta, max = 0;
	/* sum of the conductances	*/
	double csum;
	/* weighted sum of temperatures	*/
	double wsum;
	
	/* shortcuts for cell width(cw) and cell height(ch)	*/
	double cw = model->width / model->cols;
	double ch = model->height / model->rows;

	/* shortcuts	*/
	double ***v = temp->cuboid;
	thermal_config_t *c = &model->config;
	layer_t *l = model->layers;
	int nl = model->n_layers;
	int nr = model->rows;
	int nc = model->cols;
	int spidx, hsidx, metalidx, c4idx, subidx, solderidx, pcbidx;
	int model_secondary = model->config.model_secondary;
	
	if (!model->config.model_secondary) {
		spidx = nl - DEFAULT_PACK_LAYERS + LAYER_SP;
		hsidx = nl - DEFAULT_PACK_LAYERS + LAYER_SINK;
	} else {
		spidx = nl - DEFAULT_PACK_LAYERS - SEC_PACK_LAYERS + LAYER_SP;
		hsidx = nl - DEFAULT_PACK_LAYERS - SEC_PACK_LAYERS + LAYER_SINK;
		subidx = nl - SEC_PACK_LAYERS + LAYER_SUB;
		solderidx = nl - SEC_PACK_LAYERS + LAYER_SOLDER;
		pcbidx = nl - SEC_PACK_LAYERS + LAYER_PCB;
		c4idx = nl - DEFAULT_PACK_LAYERS - SEC_PACK_LAYERS - SEC_CHIP_LAYERS + LAYER_C4;
		metalidx = nl - DEFAULT_PACK_LAYERS - SEC_PACK_LAYERS - SEC_CHIP_LAYERS + LAYER_METAL;		
	}

	/* for each grid cell	*/
	for(n=0; n < nl; n++) {
		for(i=0; i < nr; i++) {
			for(j=0; j < nc; j++) {
				if((n==LAYER_SI) && model_secondary) {//top silicon layer: above is metal, beneath is TIM
					csum = NC(l,n,i,j,nl,nr,nc) + SC(l,n,i,j,nl,nr,nc) + 
					   EC(l,n,i,j,nl,nr,nc) + WC(l,n,i,j,nl,nr,nc) + 
					   1.0/l[metalidx].rz + BC(l,n,i,j,nl,nr,nc);
					wsum = NT(l,v,n,i,j,nl,nr,nc) + ST(l,v,n,i,j,nl,nr,nc) + 
					   ET(l,v,n,i,j,nl,nr,nc) + WT(l,v,n,i,j,nl,nr,nc) + 
					   v[metalidx][i][j]/l[metalidx].rz + BT(l,v,n,i,j,nl,nr,nc);
				} else if((n==(metalidx-1)) && model_secondary ) {//last TIM layer: above is silicon, beneath is spreader
					csum = NC(l,n,i,j,nl,nr,nc) + SC(l,n,i,j,nl,nr,nc) + 
					   EC(l,n,i,j,nl,nr,nc) + WC(l,n,i,j,nl,nr,nc) + 
					   1.0/l[metalidx-2].rz + 1.0/l[metalidx-1].rz;
					wsum = NT(l,v,n,i,j,nl,nr,nc) + ST(l,v,n,i,j,nl,nr,nc) + 
					   ET(l,v,n,i,j,nl,nr,nc) + WT(l,v,n,i,j,nl,nr,nc) + 
					   v[metalidx-2][i][j]/l[metalidx-2].rz + v[spidx][i][j]/l[metalidx-1].rz;
				} else if((n==spidx) && model_secondary ) {//spreader: above is TIM, beneath is sink
					csum = NC(l,n,i,j,nl,nr,nc) + SC(l,n,i,j,nl,nr,nc) + 
					   EC(l,n,i,j,nl,nr,nc) + WC(l,n,i,j,nl,nr,nc) + 
					   1.0/l[metalidx-1].rz + BC(l,n,i,j,nl,nr,nc);
					wsum = NT(l,v,n,i,j,nl,nr,nc) + ST(l,v,n,i,j,nl,nr,nc) + 
					   ET(l,v,n,i,j,nl,nr,nc) + WT(l,v,n,i,j,nl,nr,nc) + 
					   v[metalidx-1][i][j]/l[metalidx-1].rz + BT(l,v,n,i,j,nl,nr,nc);
				} else if((n==metalidx) && model_secondary) { //metal layer: above is C4, below is Si
					csum = NC(l,n,i,j,nl,nr,nc) + SC(l,n,i,j,nl,nr,nc) + 
					   EC(l,n,i,j,nl,nr,nc) + WC(l,n,i,j,nl,nr,nc) + 
					   1.0/l[c4idx].rz + 1.0/l[metalidx].rz;
					wsum = NT(l,v,n,i,j,nl,nr,nc) + ST(l,v,n,i,j,nl,nr,nc) + 
					   ET(l,v,n,i,j,nl,nr,nc) + WT(l,v,n,i,j,nl,nr,nc) + 
					   v[c4idx][i][j]/l[c4idx].rz + v[LAYER_SI][i][j]/l[n].rz;
				} else if((n==c4idx) && model_secondary) { // c4 layer: above is sub, below is metal
					csum = NC(l,n,i,j,nl,nr,nc) + SC(l,n,i,j,nl,nr,nc) + 
					   EC(l,n,i,j,nl,nr,nc) + WC(l,n,i,j,nl,nr,nc) + 
					   1.0/l[subidx].rz + 1.0/l[c4idx].rz;
					wsum = NT(l,v,n,i,j,nl,nr,nc) + ST(l,v,n,i,j,nl,nr,nc) + 
					   ET(l,v,n,i,j,nl,nr,nc) + WT(l,v,n,i,j,nl,nr,nc) + 
					   v[subidx][i][j]/l[subidx].rz + v[metalidx][i][j]/l[n].rz;
				} else if((n==subidx) && model_secondary) { //sub layer: above is solder, below is c4 
					csum = NC(l,n,i,j,nl,nr,nc) + SC(l,n,i,j,nl,nr,nc) + 
					   EC(l,n,i,j,nl,nr,nc) + WC(l,n,i,j,nl,nr,nc) + 
					   1.0/l[solderidx].rz + 1.0/l[subidx].rz;
					wsum = NT(l,v,n,i,j,nl,nr,nc) + ST(l,v,n,i,j,nl,nr,nc) + 
					   ET(l,v,n,i,j,nl,nr,nc) + WT(l,v,n,i,j,nl,nr,nc) + 
					   v[solderidx][i][j]/l[solderidx].rz + v[c4idx][i][j]/l[n].rz;
				} else if((n==solderidx) && model_secondary) { //solder layer: above is PCB, below is sub
					csum = NC(l,n,i,j,nl,nr,nc) + SC(l,n,i,j,nl,nr,nc) + 
					   EC(l,n,i,j,nl,nr,nc) + WC(l,n,i,j,nl,nr,nc) + 
					   1.0/l[pcbidx].rz + 1.0/l[solderidx].rz;
					wsum = NT(l,v,n,i,j,nl,nr,nc) + ST(l,v,n,i,j,nl,nr,nc) + 
					   ET(l,v,n,i,j,nl,nr,nc) + WT(l,v,n,i,j,nl,nr,nc) + 
					   v[pcbidx][i][j]/l[pcbidx].rz + v[subidx][i][j]/l[n].rz;
				} else if((n==pcbidx) && model_secondary) { //PCB layer, above is ambient, below is solder
					csum = NC(l,n,i,j,nl,nr,nc) + SC(l,n,i,j,nl,nr,nc) + 
					   EC(l,n,i,j,nl,nr,nc) + WC(l,n,i,j,nl,nr,nc) + 
					   1.0/l[pcbidx].rz;
					wsum = NT(l,v,n,i,j,nl,nr,nc) + ST(l,v,n,i,j,nl,nr,nc) + 
					   ET(l,v,n,i,j,nl,nr,nc) + WT(l,v,n,i,j,nl,nr,nc) + 
					   v[solderidx][i][j]/l[n].rz;
				}	else if((n==hsidx) && model_secondary) {
					csum = NC(l,n,i,j,nl,nr,nc) + SC(l,n,i,j,nl,nr,nc) + 
					   EC(l,n,i,j,nl,nr,nc) + WC(l,n,i,j,nl,nr,nc) + 
					   1.0/l[spidx].rz;
					wsum = NT(l,v,n,i,j,nl,nr,nc) + ST(l,v,n,i,j,nl,nr,nc) + 
					   ET(l,v,n,i,j,nl,nr,nc) + WT(l,v,n,i,j,nl,nr,nc) + 
					   v[spidx][i][j]/l[n-1].rz;
				}	else {
					/* sum the conductances to cells north, south, 
				 	* east, west, above and below
				 	*/
					csum = NC(l,n,i,j,nl,nr,nc) + SC(l,n,i,j,nl,nr,nc) + 
					   EC(l,n,i,j,nl,nr,nc) + WC(l,n,i,j,nl,nr,nc) + 
					   AC(l,n,i,j,nl,nr,nc) + BC(l,n,i,j,nl,nr,nc);

					/* sum of the weighted temperatures of all the neighbours	*/
					wsum = NT(l,v,n,i,j,nl,nr,nc) + ST(l,v,n,i,j,nl,nr,nc) + 
					   ET(l,v,n,i,j,nl,nr,nc) + WT(l,v,n,i,j,nl,nr,nc) + 
					   AT(l,v,n,i,j,nl,nr,nc) + BT(l,v,n,i,j,nl,nr,nc);
				}
				
				/* special treatment to C4/underfill */
				if ((n == c4idx) && model->config.model_secondary) {
					//FIXME!
				}
				/* spreader core is connected to its periphery	*/
				if (n == spidx) {
					/* northern boundary - edge cell has half the ry	*/
					if (i == 0) {
						csum += 1.0/(l[n].ry/2.0 + nc*model->pack.r_sp1_y); 
						wsum += temp->extra[SP_N]/(l[n].ry/2.0 + nc*model->pack.r_sp1_y); 
					}
					/* southern boundary - edge cell has half the ry	*/
					if (i == nr-1) {
						csum += 1.0/(l[n].ry/2.0 + nc*model->pack.r_sp1_y); 
						wsum += temp->extra[SP_S]/(l[n].ry/2.0 + nc*model->pack.r_sp1_y); 
					}
					/* eastern boundary	 - edge cell has half the rx	*/
					if (j == nc-1) {
						csum += 1.0/(l[n].rx/2.0 + nr*model->pack.r_sp1_x); 
						wsum += temp->extra[SP_E]/(l[n].rx/2.0 + nr*model->pack.r_sp1_x); 
					}
					/* western boundary	- edge cell has half the rx		*/
					if (j == 0) {
						csum += 1.0/(l[n].rx/2.0 + nr*model->pack.r_sp1_x); 
						wsum += temp->extra[SP_W]/(l[n].rx/2.0 + nr*model->pack.r_sp1_x); 
					}
				/* heatsink core is connected to its inner periphery and ambient	*/
				} else if (n == hsidx) {
					/* all nodes are connected to the ambient	*/
					csum += 1.0/l[n].rz;
					wsum += c->ambient/l[n].rz;
					/* northern boundary - edge cell has half the ry	*/
					if (i == 0) {
						csum += 1.0/(l[n].ry/2.0 + nc*model->pack.r_hs1_y); 
						wsum += temp->extra[SINK_C_N]/(l[n].ry/2.0 + nc*model->pack.r_hs1_y); 
					}
					/* southern boundary - edge cell has half the ry	*/
					if (i == nr-1) {
						csum += 1.0/(l[n].ry/2.0 + nc*model->pack.r_hs1_y); 
						wsum += temp->extra[SINK_C_S]/(l[n].ry/2.0 + nc*model->pack.r_hs1_y); 
					}
					/* eastern boundary	 - edge cell has half the rx	*/
					if (j == nc-1) {
						csum += 1.0/(l[n].rx/2.0 + nr*model->pack.r_hs1_x); 
						wsum += temp->extra[SINK_C_E]/(l[n].rx/2.0 + nr*model->pack.r_hs1_x); 
					}
					/* western boundary	- edge cell has half the rx		*/
					if (j == 0) {
						csum += 1.0/(l[n].rx/2.0 + nr*model->pack.r_hs1_x); 
						wsum += temp->extra[SINK_C_W]/(l[n].rx/2.0 + nr*model->pack.r_hs1_x); 
					}
				} else if ((n==subidx) && model->config.model_secondary) {
					/* northern boundary - edge cell has half the ry	*/
					if (i == 0) {
						csum += 1.0/(l[n].ry/2.0 + nc*model->pack.r_sub1_y); 
						wsum += temp->extra[SUB_N]/(l[n].ry/2.0 + nc*model->pack.r_sub1_y); 
					}
					/* southern boundary - edge cell has half the ry	*/
					if (i == nr-1) {
						csum += 1.0/(l[n].ry/2.0 + nc*model->pack.r_sub1_y); 
						wsum += temp->extra[SUB_S]/(l[n].ry/2.0 + nc*model->pack.r_sub1_y); 
					}
					/* eastern boundary	 - edge cell has half the rx	*/
					if (j == nc-1) {
						csum += 1.0/(l[n].rx/2.0 + nr*model->pack.r_sub1_x); 
						wsum += temp->extra[SUB_E]/(l[n].rx/2.0 + nr*model->pack.r_sub1_x); 
					}
					/* western boundary	- edge cell has half the rx		*/
					if (j == 0) {
						csum += 1.0/(l[n].rx/2.0 + nr*model->pack.r_sub1_x); 
						wsum += temp->extra[SUB_W]/(l[n].rx/2.0 + nr*model->pack.r_sub1_x); 
					} 
				} else if ((n==solderidx) && model->config.model_secondary) {
					/* northern boundary - edge cell has half the ry	*/
					if (i == 0) {
						csum += 1.0/(l[n].ry/2.0 + nc*model->pack.r_solder1_y); 
						wsum += temp->extra[SOLDER_N]/(l[n].ry/2.0 + nc*model->pack.r_solder1_y); 
					}
					/* southern boundary - edge cell has half the ry	*/
					if (i == nr-1) {
						csum += 1.0/(l[n].ry/2.0 + nc*model->pack.r_solder1_y); 
						wsum += temp->extra[SOLDER_S]/(l[n].ry/2.0 + nc*model->pack.r_solder1_y); 
					}
					/* eastern boundary	 - edge cell has half the rx	*/
					if (j == nc-1) {
						csum += 1.0/(l[n].rx/2.0 + nr*model->pack.r_solder1_x); 
						wsum += temp->extra[SOLDER_E]/(l[n].rx/2.0 + nr*model->pack.r_solder1_x); 
					}
					/* western boundary	- edge cell has half the rx		*/
					if (j == 0) {
						csum += 1.0/(l[n].rx/2.0 + nr*model->pack.r_solder1_x); 
						wsum += temp->extra[SOLDER_W]/(l[n].rx/2.0 + nr*model->pack.r_solder1_x); 
					} 
				} else if ((n==pcbidx) && model->config.model_secondary) {
					/* all nodes are connected to the ambient	*/
					csum += 1.0/(model->config.r_convec_sec * 
								   (model->config.s_pcb * model->config.s_pcb) / (cw * ch));
					wsum += c->ambient/(model->config.r_convec_sec * 
								   (model->config.s_pcb * model->config.s_pcb) / (cw * ch));
					/* northern boundary - edge cell has half the ry	*/
					if (i == 0) {
						csum += 1.0/(l[n].ry/2.0 + nc*model->pack.r_pcb1_y); 
						wsum += temp->extra[PCB_C_N]/(l[n].ry/2.0 + nc*model->pack.r_pcb1_y); 
					}
					/* southern boundary - edge cell has half the ry	*/
					if (i == nr-1) {
						csum += 1.0/(l[n].ry/2.0 + nc*model->pack.r_pcb1_y); 
						wsum += temp->extra[PCB_C_S]/(l[n].ry/2.0 + nc*model->pack.r_pcb1_y); 
					}
					/* eastern boundary	 - edge cell has half the rx	*/
					if (j == nc-1) {
						csum += 1.0/(l[n].rx/2.0 + nr*model->pack.r_pcb1_x); 
						wsum += temp->extra[PCB_C_E]/(l[n].rx/2.0 + nr*model->pack.r_pcb1_x); 
					}
					/* western boundary	- edge cell has half the rx		*/
					if (j == 0) {
						csum += 1.0/(l[n].rx/2.0 + nr*model->pack.r_pcb1_x); 
						wsum += temp->extra[PCB_C_W]/(l[n].rx/2.0 + nr*model->pack.r_pcb1_x); 
					}
				}

				/* update the current cell's temperature	*/	   
				prev = v[n][i][j];
				v[n][i][j] = (power->cuboid[n][i][j] + wsum) / csum;
				
				/* compute maximum delta	*/
				delta =  fabs(prev - v[n][i][j]);
				if (delta > max)
					max = delta;
			}
		}
	}
	/* package part of the iteration	*/
	return (MAX(max, single_iteration_steady_pack(model, power, temp)));
}

/* restriction operator for multigrid solver. given a power vector
 * corresponding to a fine grid, outputs a vector corresponding
 * to one level coarser grid (half the no. of rows and cols)
 * NOTE: model->rows and model->cols denote the size of the
 * coarser grid
 */
void multigrid_restrict_power(grid_model_t *model, grid_model_vector_t *dst, 
							  grid_model_vector_t *src)
{
	/* coarse grid indices	*/
	int n, i, j;

	/* grid cells - add the four nearest neighbours	*/
	for(n=0; n < model->n_layers; n++)
		for(i=0; i < model->rows; i++)
			for(j=0; j < model->cols; j++)
			{
				dst->cuboid[n][i][j] = (src->cuboid[n][2*i][2*j] +
										src->cuboid[n][2*i+1][2*j] +
										src->cuboid[n][2*i][2*j+1] +
										src->cuboid[n][2*i+1][2*j+1]);
			}
	/* package nodes - copy them as it is	*/
	if (!model->config.model_secondary)
		copy_dvector(dst->extra, src->extra, EXTRA);
	else
		copy_dvector(dst->extra, src->extra, EXTRA+EXTRA_SEC);
}

/* prolongation(interpolation) operator for multigrid solver. 
 * given a temperature vector corresponding to a coarse grid,
 * outputs a (bi)linearly interpolated vector corresponding 
 * to one level finer grid (twice the no. of rows and cols)
 * NOTE: model->rows and model->cols denote the size of the
 * coarser grid
 */
void multigrid_prolong_temp(grid_model_t *model, grid_model_vector_t *dst, 
							grid_model_vector_t *src)
{
	/* coarse grid indices	*/
	int n, i, j;

	/* shortcuts	*/
	int nr = model->rows;
	int nc = model->cols;
	double ***d = dst->cuboid;
	double ***s = src->cuboid;

	/* For the fine grid cells not on the boundary,
	 * we want to linearly interpolate in the region 
	 * surrounded by the coarse grid cells (i,j),
	 * (i+1,j) (i,j+1) and (i+1, j+1). The fine
	 * grid cells in that region are (2i+1, 2j+1),
	 * (2i+2, 2j+1), (2i+1, 2j+2) and (2i+2, 2j+2).
	 * To interpolate bilinearly, we first interpolate
	 * along one axis and then along the other. In
	 * each axis, due to the proximity of the fine
	 * grid cell co-ordinates to one of the coarse
	 * grid cells, the weights are 3/4 and 1/4 (not
	 * 1/2 and 1/2) So, repeating them along the  
	 * other axis also results in weights of 9/16,
	 * 3/16, 3/16 and 1/16
	 */ 
	for(n=0; n < model->n_layers; n++)
		for(i=0; i < nr-1; i++)
			for(j=0; j < nc-1; j++) {
				d[n][2*i+1][2*j+1] = 9.0/16.0 * s[n][i][j] +
									 3.0/16.0 * s[n][i+1][j] +
									 1.0/16.0 * s[n][i+1][j+1] +
									 3.0/16.0 * s[n][i][j+1];
				d[n][2*i+2][2*j+1] = 3.0/16.0 * s[n][i][j] +
									 9.0/16.0 * s[n][i+1][j] +
									 3.0/16.0 * s[n][i+1][j+1] +
									 1.0/16.0 * s[n][i][j+1];
				d[n][2*i+2][2*j+2] = 1.0/16.0 * s[n][i][j] +
									 3.0/16.0 * s[n][i+1][j] +
									 9.0/16.0 * s[n][i+1][j+1] +
									 3.0/16.0 * s[n][i][j+1];
				d[n][2*i+1][2*j+2] = 3.0/16.0 * s[n][i][j] +
									 1.0/16.0 * s[n][i+1][j] +
									 3.0/16.0 * s[n][i+1][j+1] +
									 9.0/16.0 * s[n][i][j+1];
			}

	/* for the cells on the boundary, we perform
	 * a zeroth order interpolation. i.e., copy
	 * the nearest coarse grid cell's value
	 * as it is
	 */
	for(n=0; n < model->n_layers; n++) {
		for(i=0; i < nr; i++) {
			d[n][2*i][0] = d[n][2*i+1][0] = s[n][i][0];
			d[n][2*i][2*nc-1] = d[n][2*i+1][2*nc-1] = s[n][i][nc-1];
		}	
		for(j=0; j < nc; j++) {
			d[n][0][2*j] = d[n][0][2*j+1] = s[n][0][j];
			d[n][2*nr-1][2*j] = d[n][2*nr-1][2*j+1] = s[n][nr-1][j];
		}
	}
	
	/* package nodes - copy them as it is	*/
	if (!model->config.model_secondary) 
		copy_dvector(dst->extra, src->extra, EXTRA);
	else
		copy_dvector(dst->extra, src->extra, EXTRA+EXTRA_SEC);
}

/* recursive multigrid solver. it uses the Gauss-Seidel (GS) iterative 
 * solver to solve at a particular grid granularity. Although GS removes
 * high frequency errors in a solution estimate pretty quickly, it 
 * takes a lot of time to eliminate the low frequency ones. These 
 * low frequency errors can be eliminated easily by coarsifying the
 * grid. This is the core principle of mulrigrid solvers. The version here
 * is called "nested iteration". It solves the problem (iteratively using GS)
 * first in the coarsest granularity and then utilizes that solution to 
 * estimate the solution in the next finer grid. This is repeated until
 * the solution is found in the finest desired grid. For details, take 
 * a look at Numerical Recipes in C (2nd edition), sections 19.5 and 19.6
 * (http://www.nrbook.com/a/bookcpdf/c19-5.pdf and
 * http://www.nrbook.com/a/bookcpdf/c19-6.pdf). Also refer to Prof.
 * Jayathi Murthy's ME 608 notes from Purdue - Chapter 8, sections 8.7-8.9.
 * (http://meweb.ecn.purdue.edu/~jmurthy/me608/main.pdf - pg. 175-192)
 */ 
void recursive_multigrid(grid_model_t *model, grid_model_vector_t *power,
						 grid_model_vector_t *temp)
{
	double delta;
	#if VERBOSE > 1
	unsigned int i = 0;
	#endif
	grid_model_vector_t *coarse_power, *coarse_temp;
	int n;

	/* setup heuristic initial temperatures	at the coarsest level*/
	if (model->rows <= 1 || model->cols <= 1) {
		set_heuristic_temp(model, power, temp);
		
	
	/* for finer grids. use coarser solutions as estimates	*/
	} else {
		/* make the grid coarser	*/
		model->rows /= 2;
		model->cols /= 2;
		for(n=0; n < model->n_layers; n++) {
			/* only rz's and c's change. rx's and 
			 * ry's remain the same	
			 */
			model->layers[n].rz /= 4;
			if (model->c_ready)
				model->layers[n].c *= 4;
		}

		/* vectors for the coarse grid	*/
		coarse_power = new_grid_model_vector(model);
		coarse_temp = new_grid_model_vector(model);

		/* coarsen the power vector	*/
		multigrid_restrict_power(model, coarse_power, power);

		/* solve recursively	*/
		recursive_multigrid(model, coarse_power, coarse_temp);

		/* interpolate the solution to the current fine grid	*/
		multigrid_prolong_temp(model, temp, coarse_temp);

		/* cleanup	*/
		free_grid_model_vector(coarse_power);
		free_grid_model_vector(coarse_temp);
		
		/* restore the grid */
		model->rows *= 2;
		model->cols *= 2;
		for(n=0; n < model->n_layers; n++) {
			model->layers[n].rz *= 4;
			if (model->c_ready)
				model->layers[n].c /= 4;
		}
	}
	/* refine solution iteratively till convergence	*/
	do {
		delta = single_iteration_steady_grid(model, power, temp);
		#if VERBOSE > 1
			i++;
		#endif
	} while (!eq(delta, 0));
	#if VERBOSE > 1
	fprintf(stdout, "no. of iterations for steady state convergence (%d x %d grid): %d\n", 
					model->rows, model->cols, i);
	#endif
}

void steady_state_temp_grid(grid_model_t *model, double *power, double *temp)
{
	grid_model_vector_t *p;
	double total;

	if (!model->r_ready)
		fatal("R model not ready\n");

	p = new_grid_model_vector(model);

	/* package nodes' power numbers	*/
	set_internal_power_grid(model, power);
	
	/* total power - package nodes have no power dissipation	*/
	total = sum_dvector(power, model->total_n_blocks);

	/* map the block power numbers to the grid	*/
	xlate_vector_b2g(model, power, p, V_POWER);

	/* solve recursively. use grid model's internal 
	 * state vector to store the grid temperatures
	 */ 
	recursive_multigrid(model, p, model->last_steady);

	/* map the temperature numbers back	*/
	xlate_temp_g2b(model, temp, model->last_steady);

	free_grid_model_vector(p);
}

/* function to access a 1-d array as a 3-d matrix	*/
#define A3D(array,n,i,j,nl,nr,nc)		(array[(n)*(nr)*(nc) + (i)*(nc) + (j)])

/* compute the slope vector for the package nodes	*/
void slope_fn_pack(grid_model_t *model, double *v, grid_model_vector_t *p, double *dv)
{
	int i, j;
	/* sum of the currents(power values)	*/
	double psum;
	
	/* shortcuts	*/
	package_RC_t *pk = &model->pack;
	thermal_config_t *c = &model->config;
	layer_t *l = model->layers;
	int nl = model->n_layers;
	int nr = model->rows;
	int nc = model->cols;
	int spidx, hsidx, metalidx, c4idx, subidx, solderidx, pcbidx;
	
	/* pointer to the starting address of the extra nodes	*/
	double *x = v + nl*nr*nc;

	
	if (!model->config.model_secondary) {
		spidx = nl - DEFAULT_PACK_LAYERS + LAYER_SP;
		hsidx = nl - DEFAULT_PACK_LAYERS + LAYER_SINK;
	} else {
		spidx = nl - DEFAULT_PACK_LAYERS - SEC_PACK_LAYERS + LAYER_SP;
		hsidx = nl - DEFAULT_PACK_LAYERS - SEC_PACK_LAYERS + LAYER_SINK;
		metalidx = nl - DEFAULT_PACK_LAYERS - SEC_PACK_LAYERS - SEC_CHIP_LAYERS + LAYER_METAL;
		c4idx = nl - DEFAULT_PACK_LAYERS - SEC_PACK_LAYERS - SEC_CHIP_LAYERS + LAYER_C4;
		subidx = nl - SEC_PACK_LAYERS + LAYER_SUB;
		solderidx = nl - SEC_PACK_LAYERS + LAYER_SOLDER;
		pcbidx = nl - SEC_PACK_LAYERS + LAYER_PCB;		
	}
	

	/* sink outer north/south	*/
	psum = (c->ambient - x[SINK_N])/(pk->r_hs_per + pk->r_amb_per) + 
		   (x[SINK_C_N] - x[SINK_N])/(pk->r_hs2_y + pk->r_hs);
	dv[nl*nr*nc + SINK_N] = psum / (pk->c_hs_per + pk->c_amb_per);
	psum = (c->ambient - x[SINK_S])/(pk->r_hs_per + pk->r_amb_per) + 
		   (x[SINK_C_S] - x[SINK_S])/(pk->r_hs2_y + pk->r_hs);
	dv[nl*nr*nc + SINK_S] = psum / (pk->c_hs_per + pk->c_amb_per);

	/* sink outer west/east	*/
	psum = (c->ambient - x[SINK_W])/(pk->r_hs_per + pk->r_amb_per) + 
		   (x[SINK_C_W] - x[SINK_W])/(pk->r_hs2_x + pk->r_hs);
	dv[nl*nr*nc + SINK_W] = psum / (pk->c_hs_per + pk->c_amb_per);
	psum = (c->ambient - x[SINK_E])/(pk->r_hs_per + pk->r_amb_per) + 
		   (x[SINK_C_E] - x[SINK_E])/(pk->r_hs2_x + pk->r_hs);
	dv[nl*nr*nc + SINK_E] = psum / (pk->c_hs_per + pk->c_amb_per);

	/* sink inner north/south	*/
	/* partition r_hs1_y among all the nc grid cells. edge cell has half the ry	*/
	psum = 0.0;
	for(j=0; j < nc; j++)
		psum += (A3D(v,hsidx,0,j,nl,nr,nc) - x[SINK_C_N]);
	psum /= (l[hsidx].ry / 2.0 + nc * pk->r_hs1_y);
	psum += (c->ambient - x[SINK_C_N])/(pk->r_hs_c_per_y + pk->r_amb_c_per_y) + 
			(x[SP_N] - x[SINK_C_N])/pk->r_sp_per_y +
			(x[SINK_N] - x[SINK_C_N])/(pk->r_hs2_y + pk->r_hs);
	dv[nl*nr*nc + SINK_C_N] = psum / (pk->c_hs_c_per_y + pk->c_amb_c_per_y);

	psum = 0.0;
	for(j=0; j < nc; j++)
		psum += (A3D(v,hsidx,nr-1,j,nl,nr,nc) - x[SINK_C_S]);
	psum /= (l[hsidx].ry / 2.0 + nc * pk->r_hs1_y);
	psum += (c->ambient - x[SINK_C_S])/(pk->r_hs_c_per_y + pk->r_amb_c_per_y) + 
			(x[SP_S] - x[SINK_C_S])/pk->r_sp_per_y +
			(x[SINK_S] - x[SINK_C_S])/(pk->r_hs2_y + pk->r_hs);
	dv[nl*nr*nc + SINK_C_S] = psum / (pk->c_hs_c_per_y + pk->c_amb_c_per_y);

	/* sink inner west/east	*/
	/* partition r_hs1_x among all the nr grid cells. edge cell has half the rx	*/
	psum = 0.0;
	for(i=0; i < nr; i++)
		psum += (A3D(v,hsidx,i,0,nl,nr,nc) - x[SINK_C_W]);
	psum /= (l[hsidx].rx / 2.0 + nr * pk->r_hs1_x);
	psum += (c->ambient - x[SINK_C_W])/(pk->r_hs_c_per_x + pk->r_amb_c_per_x) + 
			(x[SP_W] - x[SINK_C_W])/pk->r_sp_per_x +
			(x[SINK_W] - x[SINK_C_W])/(pk->r_hs2_x + pk->r_hs);
	dv[nl*nr*nc + SINK_C_W] = psum / (pk->c_hs_c_per_x + pk->c_amb_c_per_x);

	psum = 0.0;
	for(i=0; i < nr; i++)
		psum += (A3D(v,hsidx,i,nc-1,nl,nr,nc) - x[SINK_C_E]);
	psum /= (l[hsidx].rx / 2.0 + nr * pk->r_hs1_x);
	psum += (c->ambient - x[SINK_C_E])/(pk->r_hs_c_per_x + pk->r_amb_c_per_x) + 
			(x[SP_E] - x[SINK_C_E])/pk->r_sp_per_x +
			(x[SINK_E] - x[SINK_C_E])/(pk->r_hs2_x + pk->r_hs);
	dv[nl*nr*nc + SINK_C_E] = psum / (pk->c_hs_c_per_x + pk->c_amb_c_per_x);

	/* spreader north/south	*/
	/* partition r_sp1_y among all the nc grid cells. edge cell has half the ry	*/
	psum = 0.0;
	for(j=0; j < nc; j++)
		psum += (A3D(v,spidx,0,j,nl,nr,nc) - x[SP_N]);
	psum /= (l[spidx].ry / 2.0 + nc * pk->r_sp1_y);
	psum += (x[SINK_C_N] - x[SP_N])/pk->r_sp_per_y;
	dv[nl*nr*nc + SP_N] = psum / pk->c_sp_per_y;

	psum = 0.0;
	for(j=0; j < nc; j++)
		psum += (A3D(v,spidx,nr-1,j,nl,nr,nc) - x[SP_S]);
	psum /= (l[spidx].ry / 2.0 + nc * pk->r_sp1_y);
	psum += (x[SINK_C_S] - x[SP_S])/pk->r_sp_per_y;
	dv[nl*nr*nc + SP_S] = psum / pk->c_sp_per_y;

	/* spreader west/east	*/
	/* partition r_sp1_x among all the nr grid cells. edge cell has half the rx	*/
	psum = 0.0;
	for(i=0; i < nr; i++)
		psum += (A3D(v,spidx,i,0,nl,nr,nc) - x[SP_W]);
	psum /= (l[spidx].rx / 2.0 + nr * pk->r_sp1_x);
	psum += (x[SINK_C_W] - x[SP_W])/pk->r_sp_per_x;
	dv[nl*nr*nc + SP_W] = psum / pk->c_sp_per_x;

	psum = 0.0;
	for(i=0; i < nr; i++)
		psum += (A3D(v,spidx,i,nc-1,nl,nr,nc) - x[SP_E]);
	psum /= (l[spidx].rx / 2.0 + nr * pk->r_sp1_x);
	psum += (x[SINK_C_E] - x[SP_E])/pk->r_sp_per_x;
	dv[nl*nr*nc + SP_E] = psum / pk->c_sp_per_x;
	
	if (model->config.model_secondary) {
		/* PCB outer north/south	*/
		psum = (c->ambient - x[PCB_N])/(pk->r_amb_sec_per) + 
			   (x[PCB_C_N] - x[PCB_N])/(pk->r_pcb2_y + pk->r_pcb);
		dv[nl*nr*nc + PCB_N] = psum / (pk->c_pcb_per + pk->c_amb_sec_per);
		psum = (c->ambient - x[PCB_S])/(pk->r_amb_sec_per) + 
			   (x[PCB_C_S] - x[PCB_S])/(pk->r_pcb2_y + pk->r_pcb);
		dv[nl*nr*nc + PCB_S] = psum / (pk->c_pcb_per + pk->c_amb_sec_per);
  	
		/* PCB outer west/east	*/
		psum = (c->ambient - x[PCB_W])/(pk->r_amb_sec_per) + 
			   (x[PCB_C_W] - x[PCB_W])/(pk->r_pcb2_x + pk->r_pcb);
		dv[nl*nr*nc + PCB_W] = psum / (pk->c_pcb_per + pk->c_amb_sec_per);
		psum = (c->ambient - x[PCB_E])/(pk->r_amb_sec_per) + 
			   (x[PCB_C_E] - x[PCB_E])/(pk->r_pcb2_x + pk->r_pcb);
		dv[nl*nr*nc + PCB_E] = psum / (pk->c_pcb_per + pk->c_amb_sec_per);
  	
		/* PCB inner north/south	*/
		/* partition r_pcb1_y among all the nc grid cells. edge cell has half the ry	*/
		psum = 0.0;
		for(j=0; j < nc; j++)
			psum += (A3D(v,pcbidx,0,j,nl,nr,nc) - x[PCB_C_N]);
		psum /= (l[pcbidx].ry / 2.0 + nc * pk->r_pcb1_y);
		psum += (c->ambient - x[PCB_C_N])/(pk->r_amb_sec_c_per_y) + 
				(x[SOLDER_N] - x[PCB_C_N])/pk->r_pcb_c_per_y +
				(x[PCB_N] - x[PCB_C_N])/(pk->r_pcb2_y + pk->r_pcb);
		dv[nl*nr*nc + PCB_C_N] = psum / (pk->c_pcb_c_per_y + pk->c_amb_sec_c_per_y);
  	
		psum = 0.0;
		for(j=0; j < nc; j++)
			psum += (A3D(v,pcbidx,nr-1,j,nl,nr,nc) - x[PCB_C_S]);
		psum /= (l[pcbidx].ry / 2.0 + nc * pk->r_pcb1_y);
		psum += (c->ambient - x[PCB_C_S])/(pk->r_amb_sec_c_per_y) + 
				(x[SOLDER_S] - x[PCB_C_S])/pk->r_pcb_c_per_y +
				(x[PCB_S] - x[PCB_C_S])/(pk->r_pcb2_y + pk->r_pcb);
		dv[nl*nr*nc + PCB_C_S] = psum / (pk->c_pcb_c_per_y + pk->c_amb_sec_c_per_y);
  	
  	/* PCB inner west/east	*/
		/* partition r_pcb1_x among all the nr grid cells. edge cell has half the rx	*/
		psum = 0.0;
		for(i=0; i < nr; i++)
			psum += (A3D(v,pcbidx,i,0,nl,nr,nc) - x[PCB_C_W]);
		psum /= (l[pcbidx].rx / 2.0 + nr * pk->r_pcb1_x);
		psum += (c->ambient - x[PCB_C_W])/(pk->r_amb_sec_c_per_x) + 
				(x[SOLDER_W] - x[PCB_C_W])/pk->r_pcb_c_per_x +
				(x[PCB_W] - x[PCB_C_W])/(pk->r_pcb2_x + pk->r_pcb);
		dv[nl*nr*nc + PCB_C_W] = psum / (pk->c_pcb_c_per_x + pk->c_amb_sec_c_per_x);
  	
		psum = 0.0;
		for(i=0; i < nr; i++)
			psum += (A3D(v,pcbidx,i,nc-1,nl,nr,nc) - x[PCB_C_E]);
		psum /= (l[pcbidx].rx / 2.0 + nr * pk->r_pcb1_x);
		psum += (c->ambient - x[PCB_C_E])/(pk->r_amb_sec_c_per_x) + 
				(x[SOLDER_E] - x[PCB_C_E])/pk->r_pcb_c_per_x +
				(x[PCB_E] - x[PCB_C_E])/(pk->r_pcb2_x + pk->r_pcb);
		dv[nl*nr*nc + PCB_C_E] = psum / (pk->c_pcb_c_per_x + pk->c_amb_sec_c_per_x);
  	
		/* solder ball north/south	*/
		/* partition r_solder1_y among all the nc grid cells. edge cell has half the ry	*/
		psum = 0.0;
		for(j=0; j < nc; j++)
			psum += (A3D(v,solderidx,0,j,nl,nr,nc) - x[SOLDER_N]);
		psum /= (l[solderidx].ry / 2.0 + nc * pk->r_solder1_y);
		psum += (x[PCB_C_N] - x[SOLDER_N])/pk->r_pcb_c_per_y;
		dv[nl*nr*nc + SOLDER_N] = psum / pk->c_solder_per_y;
  	
		psum = 0.0;
		for(j=0; j < nc; j++)
			psum += (A3D(v,solderidx,nr-1,j,nl,nr,nc) - x[SOLDER_S]);
		psum /= (l[solderidx].ry / 2.0 + nc * pk->r_solder1_y);
		psum += (x[PCB_C_S] - x[SOLDER_S])/pk->r_pcb_c_per_y;
		dv[nl*nr*nc + SOLDER_S] = psum / pk->c_solder_per_y;
  	
		/* solder ball west/east	*/
		/* partition r_solder1_x among all the nr grid cells. edge cell has half the rx	*/
		psum = 0.0;
		for(i=0; i < nr; i++)
			psum += (A3D(v,solderidx,i,0,nl,nr,nc) - x[SOLDER_W]);
		psum /= (l[solderidx].rx / 2.0 + nr * pk->r_solder1_x);
		psum += (x[PCB_C_W] - x[SOLDER_W])/pk->r_pcb_c_per_x;
		dv[nl*nr*nc + SOLDER_W] = psum / pk->c_solder_per_x;
  	
		psum = 0.0;
		for(i=0; i < nr; i++)
			psum += (A3D(v,solderidx,i,nc-1,nl,nr,nc) - x[SOLDER_E]);
		psum /= (l[solderidx].rx / 2.0 + nr * pk->r_solder1_x);
		psum += (x[PCB_C_E] - x[SOLDER_E])/pk->r_pcb_c_per_x;
		dv[nl*nr*nc + SOLDER_E] = psum / pk->c_solder_per_x;
		
		/* package substrate north/south	*/
		/* partition r_sub1_y among all the nc grid cells. edge cell has half the ry	*/
		psum = 0.0;
		for(j=0; j < nc; j++)
			psum += (A3D(v,subidx,0,j,nl,nr,nc) - x[SUB_N]);
		psum /= (l[subidx].ry / 2.0 + nc * pk->r_sub1_y);
		psum += (x[SOLDER_N] - x[SUB_N])/pk->r_solder_per_y;
		dv[nl*nr*nc + SUB_N] = psum / pk->c_sub_per_y;
  	
		psum = 0.0;
		for(j=0; j < nc; j++)
			psum += (A3D(v,subidx,nr-1,j,nl,nr,nc) - x[SOLDER_S]);
		psum /= (l[subidx].ry / 2.0 + nc * pk->r_sub1_y);
		psum += (x[SOLDER_S] - x[SUB_S])/pk->r_solder_per_y;
		dv[nl*nr*nc + SUB_S] = psum / pk->c_sub_per_y;
  	
		/* sub ball west/east	*/
		/* partition r_sub1_x among all the nr grid cells. edge cell has half the rx	*/
		psum = 0.0;
		for(i=0; i < nr; i++)
			psum += (A3D(v,subidx,i,0,nl,nr,nc) - x[SUB_W]);
		psum /= (l[subidx].rx / 2.0 + nr * pk->r_sub1_x);
		psum += (x[SOLDER_W] - x[SUB_W])/pk->r_solder_per_x;
		dv[nl*nr*nc + SUB_W] = psum / pk->c_sub_per_x;
  	
		psum = 0.0;
		for(i=0; i < nr; i++)
			psum += (A3D(v,subidx,i,nc-1,nl,nr,nc) - x[SUB_E]);
		psum /= (l[subidx].rx / 2.0 + nr * pk->r_sub1_x);
		psum += (x[SOLDER_E] - x[SUB_E])/pk->r_solder_per_x;
		dv[nl*nr*nc + SUB_E] = psum / pk->c_sub_per_x;
	}
}

/* macros for calculating currents(power values)	*/
/* current(power) from the next cell north. zero if on northern boundary	*/
# define NP(l,v,n,i,j,nl,nr,nc)		((i > 0) ? ((A3D(v,n,i-1,j,nl,nr,nc)-A3D(v,n,i,j,nl,nr,nc))/l[n].ry) : 0.0)
/* current(power) from the next cell south. zero if on southern boundary	*/
# define SP(l,v,n,i,j,nl,nr,nc)		((i < nr-1) ? ((A3D(v,n,i+1,j,nl,nr,nc)-A3D(v,n,i,j,nl,nr,nc))/l[n].ry) : 0.0)
/* current(power) from the next cell east. zero if on eastern boundary	*/
# define EP(l,v,n,i,j,nl,nr,nc)		((j < nc-1) ? ((A3D(v,n,i,j+1,nl,nr,nc)-A3D(v,n,i,j,nl,nr,nc))/l[n].rx) : 0.0)
/* current(power) from the next cell west. zero if on western boundary	*/
# define WP(l,v,n,i,j,nl,nr,nc)		((j > 0) ? ((A3D(v,n,i,j-1,nl,nr,nc)-A3D(v,n,i,j,nl,nr,nc))/l[n].rx) : 0.0)
/* current(power) from the next cell below. zero if on bottom face		*/
# define BP(l,v,n,i,j,nl,nr,nc)		((n < nl-1) ? ((A3D(v,n+1,i,j,nl,nr,nc)-A3D(v,n,i,j,nl,nr,nc))/l[n].rz) : 0.0)
/* current(power) from the next cell above. zero if on top face			*/
# define AP(l,v,n,i,j,nl,nr,nc)		((n > 0) ? ((A3D(v,n-1,i,j,nl,nr,nc)-A3D(v,n,i,j,nl,nr,nc))/l[n-1].rz) : 0.0)

/* compute the slope vector for the grid cells. the transient
 * equation is CdV + sum{(T - Ti)/Ri} = P 
 * so, slope = dV = [P + sum{(Ti-T)/Ri}]/C
 */
void slope_fn_grid(grid_model_t *model, double *v, grid_model_vector_t *p, double *dv)
{
	int n, i, j;
	/* sum of the currents(power values)	*/
	double psum;
	
	/* shortcuts for cell width(cw) and cell height(ch)	*/
	double cw = model->width / model->cols;
	double ch = model->height / model->rows;

	/* shortcuts	*/
	thermal_config_t *c = &model->config;
	layer_t *l = model->layers;
	int nl = model->n_layers;
	int nr = model->rows;
	int nc = model->cols;
	int spidx, hsidx, metalidx, c4idx, subidx, solderidx, pcbidx;
	int model_secondary = model->config.model_secondary;
	
	/* pointer to the starting address of the extra nodes	*/
	double *x = v + nl*nr*nc;
	
	if (!model->config.model_secondary) {
		spidx = nl - DEFAULT_PACK_LAYERS + LAYER_SP;
		hsidx = nl - DEFAULT_PACK_LAYERS + LAYER_SINK;
	} else {
		spidx = nl - DEFAULT_PACK_LAYERS - SEC_PACK_LAYERS + LAYER_SP;
		hsidx = nl - DEFAULT_PACK_LAYERS - SEC_PACK_LAYERS + LAYER_SINK;
		metalidx = nl - DEFAULT_PACK_LAYERS - SEC_PACK_LAYERS - SEC_CHIP_LAYERS + LAYER_METAL;
		c4idx = nl - DEFAULT_PACK_LAYERS - SEC_PACK_LAYERS - SEC_CHIP_LAYERS + LAYER_C4;
		subidx = nl - SEC_PACK_LAYERS + LAYER_SUB;
		solderidx = nl - SEC_PACK_LAYERS + LAYER_SOLDER;
		pcbidx = nl - SEC_PACK_LAYERS + LAYER_PCB;		
	}
	
	/* for each grid cell	*/
	for(n=0; n < nl; n++)
		for(i=0; i < nr; i++)
			for(j=0; j < nc; j++) {
				if (n==LAYER_SI && model_secondary) { //top silicon layer
					psum = NP(l,v,n,i,j,nl,nr,nc) + SP(l,v,n,i,j,nl,nr,nc) + 
					   EP(l,v,n,i,j,nl,nr,nc) + WP(l,v,n,i,j,nl,nr,nc) + 
					   ((A3D(v,metalidx,i,j,nl,nr,nc)-A3D(v,n,i,j,nl,nr,nc))/l[metalidx].rz) +
					   ((A3D(v,n+1,i,j,nl,nr,nc)-A3D(v,n,i,j,nl,nr,nc))/l[n].rz);
				} else if (n==spidx && model_secondary) { //spreader layer
					psum = NP(l,v,n,i,j,nl,nr,nc) + SP(l,v,n,i,j,nl,nr,nc) + 
					   EP(l,v,n,i,j,nl,nr,nc) + WP(l,v,n,i,j,nl,nr,nc) + 
					   ((A3D(v,metalidx-1,i,j,nl,nr,nc)-A3D(v,n,i,j,nl,nr,nc))/l[metalidx-1].rz) +
					   ((A3D(v,hsidx,i,j,nl,nr,nc)-A3D(v,n,i,j,nl,nr,nc))/l[n].rz);
				} else if (n==metalidx && model_secondary) { //metal layer
					psum = NP(l,v,n,i,j,nl,nr,nc) + SP(l,v,n,i,j,nl,nr,nc) + 
					   EP(l,v,n,i,j,nl,nr,nc) + WP(l,v,n,i,j,nl,nr,nc) + 
					   ((A3D(v,c4idx,i,j,nl,nr,nc)-A3D(v,n,i,j,nl,nr,nc))/l[c4idx].rz) +
					   ((A3D(v,LAYER_SI,i,j,nl,nr,nc)-A3D(v,n,i,j,nl,nr,nc))/l[n].rz);
				} else if (n==metalidx-1 && model_secondary) { // TIM layer
					psum = NP(l,v,n,i,j,nl,nr,nc) + SP(l,v,n,i,j,nl,nr,nc) + 
					   EP(l,v,n,i,j,nl,nr,nc) + WP(l,v,n,i,j,nl,nr,nc) + 
					   ((A3D(v,metalidx-2,i,j,nl,nr,nc)-A3D(v,n,i,j,nl,nr,nc))/l[metalidx-2].rz) +
					   ((A3D(v,spidx,i,j,nl,nr,nc)-A3D(v,n,i,j,nl,nr,nc))/l[n].rz);
				} else if (n==c4idx && model_secondary) { //C4 layer
					psum = NP(l,v,n,i,j,nl,nr,nc) + SP(l,v,n,i,j,nl,nr,nc) + 
					   EP(l,v,n,i,j,nl,nr,nc) + WP(l,v,n,i,j,nl,nr,nc) + 
					   ((A3D(v,subidx,i,j,nl,nr,nc)-A3D(v,n,i,j,nl,nr,nc))/l[subidx].rz) +
					   ((A3D(v,metalidx,i,j,nl,nr,nc)-A3D(v,n,i,j,nl,nr,nc))/l[n].rz);
				} else if (n==subidx && model_secondary) { //Substrate layer
					psum = NP(l,v,n,i,j,nl,nr,nc) + SP(l,v,n,i,j,nl,nr,nc) + 
					   EP(l,v,n,i,j,nl,nr,nc) + WP(l,v,n,i,j,nl,nr,nc) + 
					   ((A3D(v,solderidx,i,j,nl,nr,nc)-A3D(v,n,i,j,nl,nr,nc))/l[solderidx].rz) +
					   ((A3D(v,c4idx,i,j,nl,nr,nc)-A3D(v,n,i,j,nl,nr,nc))/l[n].rz);
				} else if (n==pcbidx && model_secondary) { //PCB layer
					psum = NP(l,v,n,i,j,nl,nr,nc) + SP(l,v,n,i,j,nl,nr,nc) + 
					   EP(l,v,n,i,j,nl,nr,nc) + WP(l,v,n,i,j,nl,nr,nc) + 
					   ((A3D(v,solderidx,i,j,nl,nr,nc)-A3D(v,n,i,j,nl,nr,nc))/l[n].rz);
				} else if (n==hsidx && model_secondary) { // heatsink layer
					psum = NP(l,v,n,i,j,nl,nr,nc) + SP(l,v,n,i,j,nl,nr,nc) + 
					   EP(l,v,n,i,j,nl,nr,nc) + WP(l,v,n,i,j,nl,nr,nc) + 
					   ((A3D(v,spidx,i,j,nl,nr,nc)-A3D(v,n,i,j,nl,nr,nc))/l[spidx].rz);
				} else {
					/* sum the currents(power values) to cells north, south, 
				 	* east, west, above and below
				 	*/
					psum = NP(l,v,n,i,j,nl,nr,nc) + SP(l,v,n,i,j,nl,nr,nc) + 
					   EP(l,v,n,i,j,nl,nr,nc) + WP(l,v,n,i,j,nl,nr,nc) + 
					   AP(l,v,n,i,j,nl,nr,nc) + BP(l,v,n,i,j,nl,nr,nc);
				}

				/* spreader core is connected to its periphery	*/
				if (n == spidx) {
					/* northern boundary - edge cell has half the ry	*/
					if (i == 0)
						psum += (x[SP_N] - A3D(v,n,i,j,nl,nr,nc))/(l[n].ry/2.0 + nc*model->pack.r_sp1_y); 
					/* southern boundary - edge cell has half the ry	*/
					if (i == nr-1)
						psum += (x[SP_S] - A3D(v,n,i,j,nl,nr,nc))/(l[n].ry/2.0 + nc*model->pack.r_sp1_y); 
					/* eastern boundary	 - edge cell has half the rx	*/
					if (j == nc-1)
						psum += (x[SP_E] - A3D(v,n,i,j,nl,nr,nc))/(l[n].rx/2.0 + nr*model->pack.r_sp1_x); 
					/* western boundary	 - edge cell has half the rx	*/
					if (j == 0)
						psum += (x[SP_W] - A3D(v,n,i,j,nl,nr,nc))/(l[n].rx/2.0 + nr*model->pack.r_sp1_x); 
				/* heatsink core is connected to its inner periphery and ambient	*/
				} else if (n == hsidx) {
					/* all nodes are connected to the ambient	*/
					psum += (c->ambient - A3D(v,n,i,j,nl,nr,nc))/l[n].rz;
					/* northern boundary - edge cell has half the ry	*/
					if (i == 0)
						psum += (x[SINK_C_N] - A3D(v,n,i,j,nl,nr,nc))/(l[n].ry/2.0 + nc*model->pack.r_hs1_y); 
					/* southern boundary - edge cell has half the ry	*/
					if (i == nr-1)
						psum += (x[SINK_C_S] - A3D(v,n,i,j,nl,nr,nc))/(l[n].ry/2.0 + nc*model->pack.r_hs1_y); 
					/* eastern boundary	 - edge cell has half the rx	*/
					if (j == nc-1)
						psum += (x[SINK_C_E] - A3D(v,n,i,j,nl,nr,nc))/(l[n].rx/2.0 + nr*model->pack.r_hs1_x); 
					/* western boundary	 - edge cell has half the rx	*/
					if (j == 0)
						psum += (x[SINK_C_W] - A3D(v,n,i,j,nl,nr,nc))/(l[n].rx/2.0 + nr*model->pack.r_hs1_x); 
				}	else if (n == pcbidx && model->config.model_secondary) {
					/* all nodes are connected to the ambient	*/
					psum += (c->ambient - A3D(v,n,i,j,nl,nr,nc))/(model->config.r_convec_sec * 
								   (model->config.s_pcb * model->config.s_pcb) / (cw * ch));
					/* northern boundary - edge cell has half the ry	*/
					if (i == 0)
						psum += (x[PCB_C_N] - A3D(v,n,i,j,nl,nr,nc))/(l[n].ry/2.0 + nc*model->pack.r_pcb1_y); 
					/* southern boundary - edge cell has half the ry	*/
					if (i == nr-1)
						psum += (x[PCB_C_S] - A3D(v,n,i,j,nl,nr,nc))/(l[n].ry/2.0 + nc*model->pack.r_pcb1_y); 
					/* eastern boundary	 - edge cell has half the rx	*/
					if (j == nc-1)
						psum += (x[PCB_C_E] - A3D(v,n,i,j,nl,nr,nc))/(l[n].rx/2.0 + nr*model->pack.r_pcb1_x); 
					/* western boundary	 - edge cell has half the rx	*/
					if (j == 0)
						psum += (x[PCB_C_W] - A3D(v,n,i,j,nl,nr,nc))/(l[n].rx/2.0 + nr*model->pack.r_pcb1_x); 
				}	else if (n == subidx && model->config.model_secondary) {
					/* northern boundary - edge cell has half the ry	*/
					if (i == 0)
						psum += (x[SUB_N] - A3D(v,n,i,j,nl,nr,nc))/(l[n].ry/2.0 + nc*model->pack.r_sub1_y); 
					/* southern boundary - edge cell has half the ry	*/
					if (i == nr-1)
						psum += (x[SUB_S] - A3D(v,n,i,j,nl,nr,nc))/(l[n].ry/2.0 + nc*model->pack.r_sub1_y); 
					/* eastern boundary	 - edge cell has half the rx	*/
					if (j == nc-1)
						psum += (x[SUB_E] - A3D(v,n,i,j,nl,nr,nc))/(l[n].rx/2.0 + nr*model->pack.r_sub1_x); 
					/* western boundary	 - edge cell has half the rx	*/
					if (j == 0)
						psum += (x[SUB_W] - A3D(v,n,i,j,nl,nr,nc))/(l[n].rx/2.0 + nr*model->pack.r_sub1_x); 
				}	else if (n == solderidx && model->config.model_secondary) {
					/* northern boundary - edge cell has half the ry	*/
					if (i == 0)
						psum += (x[SOLDER_N] - A3D(v,n,i,j,nl,nr,nc))/(l[n].ry/2.0 + nc*model->pack.r_solder1_y); 
					/* southern boundary - edge cell has half the ry	*/
					if (i == nr-1)
						psum += (x[SOLDER_S] - A3D(v,n,i,j,nl,nr,nc))/(l[n].ry/2.0 + nc*model->pack.r_solder1_y); 
					/* eastern boundary	 - edge cell has half the rx	*/
					if (j == nc-1)
						psum += (x[SOLDER_E] - A3D(v,n,i,j,nl,nr,nc))/(l[n].rx/2.0 + nr*model->pack.r_solder1_x); 
					/* western boundary	 - edge cell has half the rx	*/
					if (j == 0)
						psum += (x[SOLDER_W] - A3D(v,n,i,j,nl,nr,nc))/(l[n].rx/2.0 + nr*model->pack.r_solder1_x); 
				}

				/* update the current cell's temperature	*/	   
				A3D(dv,n,i,j,nl,nr,nc) = (p->cuboid[n][i][j] + psum) / l[n].c;
			}
	slope_fn_pack(model, v, p, dv);
}

void compute_temp_grid(grid_model_t *model, double *power, double *temp, double time_elapsed)
{
	double t, h, new_h;
	int extra_nodes;
	grid_model_vector_t *p;
	#if VERBOSE > 1
	unsigned int i = 0;
	#endif
	
	if (model->config.model_secondary)
		extra_nodes = EXTRA + EXTRA_SEC;
	else
		extra_nodes = EXTRA;
	
	if (!model->r_ready || !model->c_ready)
		fatal("grid model not ready\n");

	p = new_grid_model_vector(model);

	/* package nodes' power numbers	*/
	set_internal_power_grid(model, power);

	/* map the block power/temp numbers to the grid	*/
	xlate_vector_b2g(model, power, p, V_POWER);

	/* if temp is NULL, re-use the temperature from the
	 * last call. otherwise, translate afresh and remember 
	 * the grid and block temperature arrays for future use
	 */
	if (temp != NULL) {
		xlate_vector_b2g(model, temp, model->last_trans, V_TEMP);
		model->last_temp = temp;
	}	

	/* Obtain temp at time (t+time_elapsed). 
	 * Instead of getting the temperature at t+time_elapsed directly, we
	 * do it in multiple steps with the correct step size at each time 
	 * provided by rk4. 
	 */
	for (t = 0, new_h = MIN_STEP; t + new_h <= time_elapsed; t+=h) {
		h = new_h;
		/* pass the entire grid and the tail of package nodes 
		 * as a 1-d array
		 */
		new_h = rk4(model, model->last_trans->cuboid[0][0],  p, 
				 /* array size = grid size + EXTRA	*/
				 model->rows * model->cols * model->n_layers + extra_nodes, h,
				 model->last_trans->cuboid[0][0], 
				 /* the slope function callback is typecast accordingly */
				 (slope_fn_ptr) slope_fn_grid);
		#if VERBOSE > 1
			i++;
		#endif	
	}

	/* remainder	*/
	if (time_elapsed > t)
		rk4(model, model->last_trans->cuboid[0][0],  p, 
			model->rows * model->cols * model->n_layers + extra_nodes,
			/* the slope function callback is typecast accordingly */
			time_elapsed - t, model->last_trans->cuboid[0][0], 
			(slope_fn_ptr) slope_fn_grid);

	#if VERBOSE > 1
	fprintf(stdout, "no. of rk4 calls during compute_temp: %d\n", i+1);
	#endif

	/* map the temperature numbers back	*/
	xlate_temp_g2b(model, model->last_temp, model->last_trans);

	free_grid_model_vector(p);
}

/* debug print	*/
void debug_print_blist(blist_t *head, flp_t *flp)
{
	blist_t *ptr;
	fprintf(stdout, "printing blist information...\n");
	for(ptr = head; ptr; ptr = ptr->next) {
		fprintf(stdout, "unit: %s\n", flp->units[ptr->idx].name);
		fprintf(stdout, "occupancy: %f\n", ptr->occupancy);
	}
}

void debug_print_glist(glist_t *array, flp_t *flp)
{
	int i;
	fprintf(stdout, "printing glist information...\n");
	for(i=0; i < flp->n_units; i++)
		fprintf(stdout, "unit: %s\tstartx: %d\tendx: %d\tstarty: %d\tendy: %d\n",
				flp->units[i].name, array[i].j1, array[i].j2, array[i].i1, array[i].i2);
}

void debug_print_layer(grid_model_t *model, layer_t *layer)
{
	int i, j;
	fprintf(stdout, "printing layer information...\n");
	fprintf(stdout, "no: %d\n", layer->no);
	fprintf(stdout, "has_lateral: %d\n", layer->has_lateral);
	fprintf(stdout, "has_power: %d\n", layer->has_power);
	fprintf(stdout, "k: %f\n", layer->k);
	fprintf(stdout, "thickness: %f\n", layer->thickness);
	fprintf(stdout, "sp: %f\n", layer->sp);
	fprintf(stdout, "rx: %f\try: %f\trz: %f\tc: %f\n", 
			layer->rx, layer->ry, layer->rz, layer->c);

	fprintf(stdout, "printing b2gmap information...\n");
	for(i=0; i < model->rows; i++)
		for(j=0; j < model->cols; j++) {
			fprintf(stdout, "row: %d, col: %d\n", i, j);
			debug_print_blist(layer->b2gmap[i][j], layer->flp);
		}

	fprintf(stdout, "printing g2bmap information...\n");
	debug_print_glist(layer->g2bmap, layer->flp);
}

void debug_print_grid_model_vector(grid_model_t *model, grid_model_vector_t *v, int nl, int nr, int nc)
{
	int n;
	int extra_nodes;
	
	if (model->config.model_secondary)
		extra_nodes = EXTRA + EXTRA_SEC;
	else
		extra_nodes = EXTRA;

	fprintf(stdout, "printing cuboid information...\n");
	for(n=0; n < nl; n++)
		dump_dmatrix(v->cuboid[n], nr, nc);
	fprintf(stdout, "printing extra information...\n");
	dump_dvector(v->extra, extra_nodes);
}

void debug_print_grid(grid_model_t *model)
{
	int i;
	int extra_nodes;
	
	if (model->config.model_secondary)
		extra_nodes = EXTRA + EXTRA_SEC;
	else
		extra_nodes = EXTRA;
		
	fprintf(stdout, "printing grid model information...\n");
	fprintf(stdout, "rows: %d\n", model->rows);
	fprintf(stdout, "cols: %d\n", model->cols);
	fprintf(stdout, "width: %f\n", model->width);
	fprintf(stdout, "height: %f\n", model->height);

	debug_print_package_RC(&model->pack);

	fprintf(stdout, "total_n_blocks: %d\n", model->total_n_blocks);
	fprintf(stdout, "map_mode: %d\n", model->map_mode);
	fprintf(stdout, "r_ready: %d\n", model->r_ready);
	fprintf(stdout, "c_ready: %d\n", model->c_ready);
	fprintf(stdout, "has_lcf: %d\n", model->has_lcf);

	fprintf(stdout, "printing last_steady information...\n");
	debug_print_grid_model_vector(model, model->last_steady, model->n_layers, 
								  model->rows, model->cols);

	fprintf(stdout, "printing last_trans information...\n");
	debug_print_grid_model_vector(model, model->last_trans, model->n_layers,
								  model->rows, model->cols);

	fprintf(stdout, "printing last_temp information...\n");
	if (model->last_temp)
		dump_dvector(model->last_temp, model->total_n_blocks + extra_nodes);
	else
		fprintf(stdout, "(null)\n");

	for(i=0; i < model->n_layers; i++)
		debug_print_layer(model, &model->layers[i]);

	fprintf(stdout, "base_n_units: %d\n", model->base_n_units);
}

