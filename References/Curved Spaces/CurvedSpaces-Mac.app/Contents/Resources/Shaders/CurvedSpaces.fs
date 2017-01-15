uniform sampler2D	uniTexture;

in vec2				varTextureCoordinates;
in vec4				varColor;	//	premultiplied alpha


void main()
{
	gl_FragColor = varColor * texture(uniTexture, varTextureCoordinates);
}
