in vec4 vPosition;
in vec4 vNorm;

uniform mat4 ctm;
uniform mat4 ptm;

out vec4 norm;
out vec4 var_pos;

void main() 
{    
    norm = vNorm;
    var_pos = vPosition;
  	gl_Position = ptm * ctm * vPosition;
} 
