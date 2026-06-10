#version version_number
// Input variables
// - Define layout because data comes directly from vertex data, optional but recommended
layout(location = 0) in type in_var_name; // Also known as vertex attribute
layout(location = 1) in type in_var_name2;

// Output variables
out type out_var_name;
// Fragment shader requries vertexColor
out vec4 vertexColor;

// Global value
// - can be accessed from any shader at any stage in shader program
// - whatever set to, uniform keeps value until either reset or updated
// Set as - glUniform4f(value, 0.0f, value, 0.0f, 1.0f);
uniform type uniform_name;

void main()
{
    // process inputs
    processed_data = 1.0

    // Swizzling
    vec2 someVec;
    vec4 differentVec = someVec.xyxx;
    vec3 anotherVec = differentVec.zyw;
    vec4 otherVec = someVec.xxxx + anotherVec.yxzy;

    // return to outputs
    out_var_name = processed_data
}
