uniform int checkerboard;
uniform vec4 view_pos;

vec4 light_pos = vec4(10.0, 10.0, 10.0, 1.0);
vec4 light_amb = vec4(0.2, 0.2, 0.2, 1.0);
vec4 light_dif = vec4(0.8, 0.8, 0.8, 1.0);
vec4 light_spe = vec4(0.5, 0.5, 0.5, 1.0);

vec4 mat_amb = vec4(1.0, 1.0, 1.0, 1.0);
vec4 mat_spe = vec4(0.5, 0.5, 0.5, 1.0);
vec4 mat_dif = vec4(0.7, 0.6, 0.2, 1.0);
float mat_shi = 128.0;

in vec4 norm;
in vec4 var_pos;

void main() 
{ 
	vec4 light_dir = normalize(light_pos - var_pos);
	
	if (checkerboard > 0) {
		/* Move xyz from [-1 1] to [0 2]
		 * Compute the checkerboard number in x, y and z
		 * Even sum x, y and z number implies white else black
		 * Worked out from initializing white pattern at -1, -1, -1
		 */
		double width = 4.0/checkerboard;
		vec3 translate = vec3(var_pos.x + 1, var_pos.y + 1, var_pos.z + 1); 
		int cx = int(translate.x/ width);
		int cy = int(translate.y / width);
		int cz = int(translate.z / width);
		// If even sum, white else black
		if (((cx + cy + cz) & 1) == 0)
			mat_dif = vec4(1.0, 1.0, 1.0, 0.0);
		else
			mat_dif = vec4(0.0, 0.0, 0.0, 0.0);
	} 
	
	vec4 dif_contr = mat_dif * light_dif;
	float kd = max(dot(normalize(light_dir), normalize(norm)), 0.0);
	gl_FragColor += kd*dif_contr;
	
	vec4 spe_contr = mat_spe * light_spe;
	vec4 eye_dir = vec4( normalize(view_pos - var_pos).xyz, 0.0 );
	vec4 h = normalize(light_dir + eye_dir);
	float ks = max(dot(h, normalize(norm)), 0.0);
	gl_FragColor += pow(ks, mat_shi) * spe_contr;
	
	vec4 amb_contr = mat_amb * light_amb;	
    gl_FragColor += amb_contr;
    gl_FragColor.a = 1.0;
} 

