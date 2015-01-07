
// the positions are sent to the GPU in a VBO:
attribute vec4 vPosition;

// same with the normals
attribute vec4 vNormal;

// we are going to output the color, which will be interoplated in the 
// fragment shader:
varying vec4 color;


// what shading mode should we use? 0 for blinn/phong,
// 1 for vertex position, 2 for vertex normals. just to
// demo connecting logic from CPU code.
uniform int shademode;

// the camera transform:
uniform mat4 cam_xform;

// the projective transform:
uniform mat4 persp_xform;

// where the camera is located, needed for shading:
uniform vec4 eye_pos;

// allow the CPU to tell us the material shinyness (using the "f" key)
uniform float material_shinyness;

// light positions (needed for shading) and the material spec:
vec4 light_position = vec4(100., 100., 100., 1.0);
vec4 light_ambient  = vec4(0.2, 0.2, 0.2, 1.0);
vec4 light_diffuse  = vec4(.80, .80, .80, 1.0);
vec4 light_specular = vec4(5.0, 5.0, 5.0, 1.0);

vec4 material_ambient  = vec4(1.0, 0.0, 1.0, 1.0);
vec4 material_diffuse  = vec4(1.0, 0.8, 0.0, 1.0);
vec4 material_specular = vec4(1.0, 1.0, 1.0, 1.0);


// shader main:
void main() 
{

  // do the basic lighting: we're going to do it in world space.
  // you could just as well do it in camera space. but you *have*
  // to do it before applying the projective transformation, which
  // warps space so that the usual geometric rules used to derive
  // the lighting equations do not hold (e.g. dot product to compute
  // cosine, etc.)
  
  // normalize the vectors to the viewer and the light source:
  vec4 light_dir = normalize(light_position - vPosition);
  vec4 eye_dir = normalize(eye_pos - vPosition);
  
  // we now have the normal (vNormal), the direction from the vertex to the 
  // light (light_dir), and the direction from the vertex to the eye (eye_dir)
  // in other words, everything we need to do the lighting calculation.
  

  // compute these three terms:
  vec4 ambient_color, diffuse_color, specular_color;
  
  // first, the diffuse:
  float dd = max (0.0, dot(light_dir, vNormal));

  diffuse_color = dd * (light_diffuse * material_diffuse);
  
  // next, the ambient:
  ambient_color = light_ambient * material_ambient;

  
  // now, the specular:
  float sd = 0.0;  

  // we only compute the specular if the light and eyepoint are on
  // the "normal" side of this vertex.
  if ((dot(light_dir, vNormal) > 0.0) && (dot(eye_dir, vNormal) > 0.0)) {
    vec4 half_vec = normalize(light_dir+eye_dir);
    sd = max(0.0, dot(half_vec, vNormal));
  }
  
  if (sd > 0.0) {
    sd = pow(sd, material_shinyness);
  }

  specular_color = sd * (light_specular * material_specular);

  // switch what we are displaying based on the shademode, which is
  // sent from the CPU:
  if ((shademode == 0) || (shademode > 5))
    color = diffuse_color + ambient_color + specular_color;
  else if (shademode == 1)
    color = diffuse_color;
  else if (shademode == 2)
    color = specular_color;
  else if (shademode == 3)
    color = ambient_color;
  else if (shademode == 4)
    color = vPosition;
  else if (shademode == 5) 
    color = vNormal;
  
  color.a = 1.0;
  
  // apply the projective transformation and the camera transformation, 
  // bringing everything into the 2x2x2 clipping space:
  gl_Position = persp_xform * cam_xform * vPosition;

} 
