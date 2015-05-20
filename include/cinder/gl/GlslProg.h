/*
 Copyright (c) 2010, The Cinder Project
 All rights reserved.
 
 This code is designed for use with the Cinder C++ library, http://libcinder.org

 Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and
	the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
	the following disclaimer in the documentation and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <map>
#include <set>

#include "cinder/gl/wrapper.h"
#include "cinder/Vector.h"
#include "cinder/Matrix.h"
#include "cinder/DataSource.h"
#include "cinder/GeomIo.h"
#include "cinder/Exception.h"

//! Convenience macro that allows one to embed raw glsl code in-line. The \a VERSION parameter will be used for the glsl's '#version' define.
//! \note Some strings will confuse different compilers, most commonly being preprocessor derictives (hence the need for \a VERSION to be a pamaeter).
//! If available on all target platforms, users should use C+11 raw string literals, which do not suffer from the same limitations.
#define CI_GLSL(VERSION,CODE) "#version " #VERSION "\n" #CODE

namespace cinder { namespace gl {
	
typedef std::shared_ptr<class GlslProg> GlslProgRef;
	
class UniformValueCache;
class ShaderPreprocessor;

class GlslProg {
  public:
	struct Attribute {
		//! Returns a const reference of the name as defined in the Vertex Shader.
		const std::string&	getName() const { return mName; }
		//! Returns the number of attributes expected by the Vertex Shader. mCount will be
		//! 1 unless this attribute is an array.
		GLint				getCount() const { return mCount; }
		//! Returns the Vertex Shader generated or user defined location of this attribute.
		GLint				getLocation() const { return mLoc; }
		//! Returns the GLenum representation of the type of this attribute (for example, \c GL_FLOAT_VEC3)
		GLenum				getType() const { return mType; }
		//! Returns the defined geom::Attrib semantic.
		geom::Attrib		getSemantic() const { return mSemantic; }
		//! Used to derive the expected layout for cpu types within glsl.
		static void getShaderAttribLayout( GLenum type, uint32_t *numDimsPerVertexPointer, uint32_t *numLocationsExpected );

	  private:
		std::string		mName;
		GLint			mCount = 0, mLoc = -1;
		GLenum			mType = -1;
		geom::Attrib	mSemantic = geom::Attrib::USER_DEFINED;

		friend class GlslProg;
	};
	
	struct Uniform {
		//! Returns a const reference of the name as defined inside the Glsl.
		const std::string&	getName() const { return mName; }
		//! Returns the number of uniforms expected by the Glsl. mCount will be
		//! 1 unless this uniform is an array.
		GLint				getCount() const { return mCount; }
		//! Returns the Glsl generated location of this uniform. If this uniform
		//! is located in a UniformBlock, it's Location will be -1.
		GLint				getLocation() const { return mLoc; }
		//! Returns the Index generated by the Glsl for this uniform. Useful for
		//! Querying Glsl about this active uniform
		GLint				getIndex() const { return mIndex; }
		//! Returns the GLenum representation of the type of this uniform (for example, \c GL_FLOAT_VEC3)
		GLenum				getType() const { return mType; }
		//! Returns the defined UniformSemantic.
		UniformSemantic		getUniformSemantic() const { return mSemantic; }
		
	  private:
		std::string		mName;
		GLint			mCount = 0, mLoc = -1, mIndex = -1;
		GLenum			mType = -1;
		UniformSemantic mSemantic = UniformSemantic::UNIFORM_USER_DEFINED;
		
		//! Used internally for the value cache. Size of a single element.
		GLint			mTypeSize = 0;
		//! Used internally for the value cache.
		GLint			mBytePointer = 0;
		
		friend class GlslProg;
	};
	
#if ! defined( CINDER_GL_ES_2 )
	struct UniformBlock {
		
		//! Returns a const reference of the name as defined inside the Glsl.
		const std::string&	getName() const { return mName; }
		//! Returns the implementation-dependent minimum total buffer object
		//! size, in basic machine units, required to hold all active uniforms
		//! in the uniform block identified by this uniform block.
		GLint				getDataSize() const { return mDataSize; }
		//! Returns the Glsl generated location of this uniform block.
		GLint				getLocation() const { return mLoc; }
		//! Returns the Glsl generated or user generated block binding of this
		//! uniform block.
		GLint				getBlockBinding() const { return mBlockBinding; }
		
		//! Returns a const reference of all the active uniforms within this uniform block.
		const std::vector<Uniform>& getActiveUniforms() const { return mActiveUniforms; }
		//! Returns 3 different aspects of this block for more advanced partitioning of data. Contains
		//! GL_UNIFORM_OFFSET - array of offsets, in basic machine units, relative to the beginning
		//! of the uniform block in the buffer object data store, GL_UNIFORM_ARRAY_STRIDE -
		//! the stride is the difference, in basic machine units, of consecutive elements in
		//! an array, or zero for uniforms not declared as an array, AND GL_UNIFORM_MATRIX_STRIDE -
		//! the stride between columns of a column-major matrix or rows of a row-major matrix,
		//! in basic machine units, of each of the active uniforms, or zero for uniforms not
		//! declared as an array.
		std::map<GLenum, std::vector<GLint>> getActiveUniformInfo() const { return mActiveUniformInfo; }
		
	  private:
		std::string				mName;
		GLint					mDataSize = 0, mLoc = -1, mBlockBinding;
		std::vector<Uniform>	mActiveUniforms;
		std::map<GLenum, std::vector<GLint>> mActiveUniformInfo;
		
		friend class GlslProg;
	};
	
	struct TransformFeedbackVaryings {
		
		//! Returns a const reference of the name as defined inside the Glsl.
		const std::string&	getName() const { return mName; }
		//! Returns the number of transform feedback varying expected by the Glsl.
		//! mCount will be 1 unless this uniform is an array.
		GLint				getCount() const { return mCount; }
		//! Returns the GLenum representation of the type of this transform feedback varying (for example, \c GL_FLOAT_VEC3)
		GLenum				getType() const { return mType; }
		
	  private:
		std::string		mName;
		GLint			mCount = 0;
		GLenum			mType;
		
		friend class GlslProg;
	};
#endif

	struct Format {
		//! Defaults to specifying location 0 for the \c geom::Attrib::POSITION semantic
		Format();
		
		//! Supplies the GLSL source for the vertex shader
		Format&		vertex( const DataSourceRef &dataSource );
		//! Supplies the GLSL source for the vertex shader
		Format&		vertex( const std::string &vertexShader );
		//! Supplies the GLSL source for the fragment shader
		Format&		fragment( const DataSourceRef &dataSource );
		//! Supplies the GLSL source for the fragment shader
		Format&		fragment( const std::string &vertexShader );
#if ! defined( CINDER_GL_ES )
		//! Supplies the GLSL source for the geometry shader
		Format&		geometry( const DataSourceRef &dataSource );
		//! Supplies the GLSL source for the geometry shader
		Format&		geometry( const std::string &geometryShader );
		//! Supplies the GLSL source for the tessellation control shader
		Format&		tessellationCtrl( const DataSourceRef &dataSource );
		//! Supplies the GLSL source for the tessellation control shader
		Format&		tessellationCtrl( const std::string &tessellationCtrlShader );
		//! Supplies the GLSL source for the tessellation control shader
		Format&		tessellationEval( const DataSourceRef &dataSource );
		//! Supplies the GLSL source for the tessellation control shader
		Format&		tessellationEval( const std::string &tessellationEvalShader );
#endif
#if ! defined( CINDER_GL_ES_2 )		
		//! Sets the TransformFeedback varyings
		Format&		feedbackVaryings( const std::vector<std::string>& varyings ) { mTransformVaryings = varyings; return *this; }
		//! Sets the TransformFeedback format
		Format&		feedbackFormat( GLenum format ) { mTransformFormat = format; return *this; }
#endif
		
		//! Specifies an attribute name to map to a specific semantic
		Format&		attrib( geom::Attrib semantic, const std::string &attribName );
		//! Specifies a uniform name to map to a specific semantic
		Format&		uniform( UniformSemantic semantic, const std::string &attribName );
		//! Specifies a location for a specific named attribute
		Format&		attribLocation( const std::string &attribName, GLint location );
		//! Specifies a location for a semantic
		Format&		attribLocation( geom::Attrib attr, GLint location );

#if ! defined( CINDER_GL_ES )
		//! Specifies a binding for a user-defined varying out variable to a fragment shader color number. Analogous to glBindFragDataLocation.
		Format&		fragDataLocation( GLuint colorNumber, const std::string &name );
#endif

		//! Returns the GLSL source for the vertex shader. Returns an empty string if it isn't present.
		const std::string&	getVertex() const { return mVertexShader; }
		//! Returns the GLSL source for the fragment shader. Returns an empty string if it isn't present.
		const std::string&	getFragment() const { return mFragmentShader; }
#if ! defined( CINDER_GL_ES )
		//! Returns the GLSL source for the geometry shader
		const std::string&	getGeometry() const { return mGeometryShader; }
		const std::string&	getTessellationCtrl() const { return mTessellationCtrlShader; }
		const std::string&	getTessellationEval() const { return mTessellationEvalShader; }
#endif
#if ! defined( CINDER_GL_ES_2 )
		const std::vector<std::string>&  getVaryings() const { return mTransformVaryings; }
		//! Returns the TransFormFeedback format
		GLenum			getTransformFormat() const { return mTransformFormat; }
		//! Returns the map between output variable names and their bound color numbers
		const std::map<std::string,GLuint>&	getFragDataLocations() const { return mFragDataLocations; }
#endif
		
		//! Returns whether preprocessing is enabled or not, e.g. `#include` statements. \default true.
		bool		isPreprocessingEnabled() const				{ return mPreprocessingEnabled; }
		//! Sets whether preprocessing is enabled or not, e.g. `#include` statements.
		void		setPreprocessingEnabled( bool enable )		{ mPreprocessingEnabled = enable; }
		//! Sets whether preprocessing is enabled or not, e.g. `#include` statements.
		Format&		preprocess( bool enable )					{ mPreprocessingEnabled = enable; return *this; }
		//! Specifies a define directive to add to the shader sources
		Format&		define( const std::string &define );
		//! Specifies a define directive to add to the shader sources
		Format&		define( const std::string &define, const std::string &value );
		//! Specifies a series of define directives to add to the shader sources
		Format&		defineDirectives( const std::vector<std::string> &defines );
		//! Specifies the #version directive to add to the shader sources
		Format&		version( int version );
		//! Returns the version number associated with this GlslProg, or 0 if none was speciefied.
		int	getVersion() const										{ return mVersion; }
		//! Returns the list of `#define` directives.
		const std::vector<std::string>& getDefineDirectives() const { return mDefineDirectives; }
		//! Adds a custom search directory to the ShaderPreprocessor's search list.
		Format&	addPreprocessorSearchDirectory( const fs::path &dir )	{ mPreprocessorSearchDirectories.push_back( dir ); return *this; }
		
		//! Returns the debugging label associated with the Program.
		const std::string&	getLabel() const { return mLabel; }
		//! Sets the debugging label associated with the Program. Calls glObjectLabel() when available.
		void				setLabel( const std::string &label ) { mLabel = label; }
		//! Sets the debugging label associated with the Program. Calls glObjectLabel() when available.
		Format&				label( const std::string &label ) { setLabel( label ); return *this; }
        
		//! Returns the fs::path for the vertex shader. Returns an empty fs::path if it isn't present.
		const fs::path&	getVertexPath() const { return mVertexShaderPath; }
		//! Returns the fs::path for the fragment shader. Returns an empty fs::path if it isn't present.
		const fs::path&	getFragmentPath() const { return mFragmentShaderPath; }
#if ! defined( CINDER_GL_ES )
		//! Returns the fs::path for the geometry shader. Returns an empty fs::path if it isn't present.
		const fs::path&	getGeometryPath() const { return mGeometryShaderPath; }
		//! Returns the fs::path for the tessellation control shader. Returns an empty fs::path if it isn't present.
		const fs::path&	getTessellationCtrlPath() const { return mTessellationCtrlShaderPath; }
		//! Returns the fs::path for the tessellation eval shader. Returns an empty fs::path if it isn't present.
		const fs::path&	getTessellationEvalPath() const { return mTessellationEvalShaderPath; }
#endif
		const std::vector<Uniform>&		getUniforms() const { return mUniforms; }
		const std::vector<Attribute>&	getAttributes() const { return mAttributes; }
		std::vector<Uniform>&			getUniforms() { return mUniforms; }
		std::vector<Attribute>&			getAttributes() { return mAttributes; }
		
	  protected:
		void			setShaderSource( const DataSourceRef &dataSource, std::string *shaderSourceDest, fs::path *shaderPathDest );
		void			setShaderSource( const std::string &source, std::string *shaderSourceDest, fs::path *shaderPathDest );

		std::string		mVertexShader;
		std::string		mFragmentShader;

		fs::path		mVertexShaderPath;
		fs::path		mFragmentShaderPath;

#if ! defined( CINDER_GL_ES )
		std::string		mGeometryShader;
		std::string		mTessellationCtrlShader;
		std::string		mTessellationEvalShader;
		fs::path		mGeometryShaderPath;
		fs::path		mTessellationCtrlShaderPath;
		fs::path		mTessellationEvalShaderPath;
#endif
#if ! defined( CINDER_GL_ES_2 )
		GLenum									mTransformFormat;
		std::vector<std::string>				mTransformVaryings;
		std::map<std::string,GLuint>			mFragDataLocations;
#endif
		std::vector<Attribute>					mAttributes;
		std::vector<Uniform>					mUniforms;
		
		std::vector<std::string>				mDefineDirectives;
		int										mVersion;
		
		bool									mPreprocessingEnabled;
		std::string								mLabel;
		std::vector<fs::path>					mPreprocessorSearchDirectories;

		friend class		GlslProg;
	};
  
	typedef std::map<std::string,UniformSemantic>	UniformSemanticMap;
	typedef std::map<std::string,geom::Attrib>		AttribSemanticMap;

	static GlslProgRef create( const Format &format );

#if ! defined( CINDER_GL_ES )
	static GlslProgRef create( DataSourceRef vertexShader,
								   DataSourceRef fragmentShader = DataSourceRef(),
								   DataSourceRef geometryShader = DataSourceRef(),
								   DataSourceRef tessEvalShader = DataSourceRef(),
								   DataSourceRef tessCtrlShader = DataSourceRef() );
	static GlslProgRef create( const std::string &vertexShader,
								   const std::string &fragmentShader = std::string(),
								   const std::string &geometryShader = std::string(),
								   const std::string &tessEvalShader = std::string(),
								   const std::string &tessCtrlShader = std::string() );
#else
	static GlslProgRef create( DataSourceRef vertexShader, DataSourceRef fragmentShader = DataSourceRef() );
	static GlslProgRef create( const std::string &vertexShader, const std::string &fragmentShader = std::string() );
#endif
	~GlslProg();
	
	void			bind() const;
	
	GLuint			getHandle() const { return mHandle; }
	
	void	uniform( const std::string &name, bool data ) const;
	void	uniform( const std::string &name, int data ) const;
	void	uniform( const std::string &name, float data ) const;
#if ! defined( CINDER_GL_ES_2 )
	void	uniform( const std::string &name, uint32_t data ) const;
	void	uniform( int location, uint32_t data ) const;
#endif
	void	uniform( int location, bool data ) const;
	void	uniform( int location, int data ) const;
	void	uniform( int location, float data ) const;
	void	uniform( const std::string &name, const vec2 &data ) const;
	void	uniform( const std::string &name, const vec3 &data ) const;
	void	uniform( const std::string &name, const vec4 &data ) const;
	void	uniform( int location, const vec2 &data ) const;
	void	uniform( int location, const vec3 &data ) const;
	void	uniform( int location, const vec4 &data ) const;
	void	uniform( const std::string &name, const ivec2 &data ) const;
	void	uniform( const std::string &name, const ivec3 &data ) const;
	void	uniform( const std::string &name, const ivec4 &data ) const;
	void	uniform( int location, const ivec2 &data ) const;
	void	uniform( int location, const ivec3 &data ) const;
	void	uniform( int location, const ivec4 &data ) const;
#if ! defined( CINDER_GL_ES_2 )
	void	uniform( const std::string &name, const uvec2 &data ) const;
	void	uniform( const std::string &name, const uvec3 &data ) const;
	void	uniform( const std::string &name, const uvec4 &data ) const;
	void	uniform( int location, const uvec2 &data ) const;
	void	uniform( int location, const uvec3 &data ) const;
	void	uniform( int location, const uvec4 &data ) const;
#endif // ! defined( CINDER_GL_ES_2 )
	void	uniform( const std::string &name, const mat2 &data, bool transpose = false ) const;
	void	uniform( const std::string &name, const mat3 &data, bool transpose = false ) const;
	void	uniform( const std::string &name, const mat4 &data, bool transpose = false ) const;
	void	uniform( int location, const mat2 &data, bool transpose = false ) const;
	void	uniform( int location, const mat3 &data, bool transpose = false ) const;
	void	uniform( int location, const mat4 &data, bool transpose = false ) const;

#if ! defined( CINDER_GL_ES_2 )
	void	uniform( const std::string &name, const uint32_t *data, int count ) const;
	void	uniform( int location, const uint32_t *data, int count ) const;
#endif // ! defined( CINDER_GL_ES_2 )
	void	uniform( const std::string &name, const int *data, int count ) const;
	void	uniform( int location, const int *data, int count ) const;
	void	uniform( const std::string &name, const float *data, int count ) const;
	void	uniform( int location, const float *data, int count ) const;
	void	uniform( const std::string &name, const ivec2 *data, int count ) const;
	void	uniform( const std::string &name, const vec2 *data, int count ) const;
	void	uniform( const std::string &name, const vec3 *data, int count ) const;
	void	uniform( const std::string &name, const vec4 *data, int count ) const;
	void	uniform( int location, const ivec2 *data, int count ) const;
	void	uniform( int location, const vec2 *data, int count ) const;
	void	uniform( int location, const vec3 *data, int count ) const;
	void	uniform( int location, const vec4 *data, int count ) const;
	void	uniform( const std::string &name, const mat2 *data, int count, bool transpose = false ) const;
	void	uniform( const std::string &name, const mat3 *data, int count, bool transpose = false ) const;
	void	uniform( const std::string &name, const mat4 *data, int count, bool transpose = false ) const;
	void	uniform( int location, const mat2 *data, int count, bool transpose = false ) const;
	void	uniform( int location, const mat3 *data, int count, bool transpose = false ) const;
	void	uniform( int location, const mat4 *data, int count, bool transpose = false ) const;
	
	bool	hasAttribSemantic( geom::Attrib semantic ) const;
	GLint	getAttribSemanticLocation( geom::Attrib semantic ) const;
	GLint	operator[]( geom::Attrib sem ) const { return getAttribSemanticLocation( sem ); }
	
	//! Default mapping from uniform name to semantic. Can be modified via the reference. Not thread-safe.
	static UniformSemanticMap&		getDefaultUniformNameToSemanticMap();
	//! Default mapping from attribute name to semantic. Can be modified via the reference. Not thread-safe.
	static AttribSemanticMap&		getDefaultAttribNameToSemanticMap();
	
	//! Returns the attrib location of the Attribute that matches \a name.
	GLint							getAttribLocation( const std::string &name ) const;
	//! Returns a const reference to the Active Attribute cache.
	const std::vector<Attribute>&	getActiveAttributes() const { return mAttributes; }
	//! Returns a const pointer to the Attribute that matches \a name. Returns nullptr if the attrib doesn't exist.
	const Attribute*	findAttrib( const std::string &name ) const;
	//! Returns a const pointer to the Attribute that matches \a semantic. Returns nullptr if the attrib doesn't exist.
	const Attribute*	findAttrib( geom::Attrib semantic ) const;
	//! Returns the uniform location of the Uniform that matches \a name.
	GLint							getUniformLocation( const std::string &name ) const;
	//! Returns a const reference to the Active Uniform cache.
	const std::vector<Uniform>&		getActiveUniforms() const { return mUniforms; }
	//! Returns a const pointer to the Uniform that matches \a name. Returns nullptr if the uniform doesn't exist. The uniform location (accounting for indices, like "example[2]") is stored in \a resultLocation if it's non-null.
	const Uniform*					findUniform( const std::string &name, int *resultLocation ) const;

#if ! defined( CINDER_GL_ES_2 )
	// Uniform blocks
	//! Analogous to glUniformBlockBinding()
	void	uniformBlock( const std::string &name, GLint binding );
	//! Analogous to glUniformBlockBinding()
	void	uniformBlock( GLint loc, GLint binding );
	//!	Returns the uniform block location of the Uniform Block that matches \a name.
	GLint	getUniformBlockLocation( const std::string &name ) const;
	//! Returns the size of the Uniform block matching \a blockIndex.
	GLint	getUniformBlockSize( GLint blockIndex ) const;
	//! Returns a const pointer to the UniformBlock that matches \a name. Returns nullptr if the uniform block doesn't exist.
	const UniformBlock* findUniformBlock( const std::string &name ) const;
	//! Returns a const reference to the UniformBlock cache.
	const std::vector<UniformBlock>& getActiveUniformBlocks() const { return mUniformBlocks; }
	//! Returns a const pointer to the TransformFeedbackVarying that matches \a name. Returns nullptr if the transform feedback varying doesn't exist.
	const TransformFeedbackVaryings* findTransformFeedbackVaryings( const std::string &name ) const;
	//! Returns a const reference to the TransformFeedbackVaryings cache.
	const std::vector<TransformFeedbackVaryings>& getActiveTransformFeedbackVaryings() const { return mTransformFeedbackVaryings; }
#endif
	
	std::string		getShaderLog( GLuint handle ) const;

	//! Returns a list of included files (via the `#include` directive) detected and parsed by the ShaderPreprocessor.
	std::vector<fs::path>	getIncludedFiles() const	{ return mShaderPreprocessorIncludedFiles; }

	//! Returns the debugging label associated with the Program.
	const std::string&	getLabel() const { return mLabel; }
	//! Sets the debugging label associated with the Program. Calls glObjectLabel() when available.
	void				setLabel( const std::string &label );

  protected:
	GlslProg( const Format &format );

	void			bindImpl() const;
	void			loadShader( const std::string &shaderSource, const fs::path &shaderPath, GLint shaderType );
	void			attachShaders();
	void			link();
	
	//! Caches all active Attributes after linking.
	void			cacheActiveAttribs();
	//! Returns a pointer to the Attribute that matches \a name. Returns nullptr if the attrib doesn't exist.
	Attribute*		findAttrib( const std::string &name );
	//! Caches all active Uniforms after linrking.
	void			cacheActiveUniforms();
	//! Returns a pointer to the Uniform that matches \a location. Returns nullptr if the uniform doesn't exist.
	const Uniform*	findUniform( int location, int *resultLocation ) const;
	
	//! Performs the finding, validation, and implementation of single uniform variables. Ends by calling the location
	//! variant uniform function.
	template<typename LookUp, typename T>
	void			uniformImpl( const LookUp &lookUp, const T &data ) const;
	template<typename T>
	void			uniformFunc( int location, const T &data ) const;
	//! Performs the finding, validation, and implementation of single uniform Matrix variables. Ends by calling the location
	//! variant uniform function.
	template<typename LookUp, typename T>
	void			uniformMatImpl( const LookUp &lookUp, const T &data, bool transpose ) const;
	template<typename T>
	void			uniformMatFunc( int location, const T &data, bool transpose ) const;
	//! Performs the finding, validation, and implementation of multiple uniform variables. Ends by calling the location
	//! variant uniform function.
	template<typename LookUp, typename T>
	void			uniformImpl( const LookUp &lookUp, const T *data, int count ) const;
	template<typename T>
	void			uniformFunc( int location, const T *data, int count ) const;
	//! Performs the finding, validation, and implementation of multiple uniform Matrix variables. Ends by calling the location
	//! variant uniform function.
	template<typename LookUp, typename T>
	void			uniformMatImpl( const LookUp &lookUp, const T *data, int count, bool transpose ) const;
	template<typename T>
	void			uniformMatFunc( int location, const T *data, int count, bool transpose ) const;
	
	//! Logs an error and caches the name.
	void			logMissingUniform( const std::string &name ) const;
	//! Logs an error and caches the name.
	void			logMissingUniform( int location ) const;
	//! Logs a warning and caches the name.
	void			logUniformWrongType( const std::string &name, GLenum uniformType, const std::string &userType ) const;
	//! Checks the validity of the settings on this uniform, specifically type and value
	template<typename T>
	bool			validateUniform( const Uniform &uniform, int uniformLocation, const T &val ) const;
	//! Checks the validity of the settings on this uniform, specifically type and value
	template<typename T>
	bool			validateUniform( const Uniform &uniform, int uniformLocation, const T *val, int count ) const;
	//! Implementing later for CPU Uniform Buffer Cache.
	bool			checkUniformValueCache( const Uniform &uniform, int location, const void *val, int count ) const;
	//! Checks the type of the uniform against the provided type of data in validateUniform. If the provided
	//! type, \a uniformType, and the type T match this function returns true, otherwise it returns false.
	template<typename T>
	bool			checkUniformType( GLenum uniformType ) const;
	template<typename T>
	bool			checkUniformType( GLenum uniformType, std::string &typeName ) const;
#if ! defined( CINDER_GL_ES_2 )
	//! Caches all active Uniform Blocks after linking.
	void			cacheActiveUniformBlocks();
	//! Returns a pointer to the Uniform Block that matches \a name. Returns nullptr if the attrib doesn't exist.
	UniformBlock*	findUniformBlock( const std::string &name );
	//! Caches all active Transform Feedback Varyings after linking.
	void			cacheActiveTransformFeedbackVaryings();
	//! Returns a pointer to the Transform Feedback Varyings that matches \a name. Returns nullptr if the attrib doesn't exist.
	TransformFeedbackVaryings* findTransformFeedbackVaryings( const std::string &name );
#endif

	template<typename T>
	static std::string	cppTypeToGlslTypeName();
	
	GLuint									mHandle;
	
	std::vector<Attribute>						mAttributes;
	std::vector<Uniform>						mUniforms;
	mutable std::unique_ptr<UniformValueCache>	mUniformValueCache;
#if ! defined( CINDER_GL_ES_2 )
	std::vector<UniformBlock>				mUniformBlocks;
	std::vector<TransformFeedbackVaryings>  mTransformFeedbackVaryings;
	GLenum									mTransformFeedbackFormat;
#endif
	
	// enumerates the uniforms we've already logged as missing so that we don't flood the log with the same message
	mutable std::set<std::string>			mLoggedUniformNames;
	mutable std::set<int>					mLoggedUniformLocations;
	std::string								mLabel; // debug label
	std::unique_ptr<ShaderPreprocessor>		mShaderPreprocessor;
	std::vector<fs::path>					mShaderPreprocessorIncludedFiles;

	friend class Context;
	friend std::ostream& operator<<( std::ostream &os, const GlslProg &rhs );
};

class GlslProgExc : public cinder::gl::Exception {
  public:
	GlslProgExc()	{}
	GlslProgExc( const std::string &description ) : cinder::gl::Exception( description )	{}
};

class GlslProgCompileExc : public GlslProgExc {
  public:
	GlslProgCompileExc( const std::string &log, GLint shaderType );
};

class GlslProgLinkExc : public GlslProgExc {
  public:
	GlslProgLinkExc( const std::string &log ) : GlslProgExc( log ) {}
};

class GlslNullProgramExc : public GlslProgExc {
  public:
	virtual const char* what() const throw()
	{
		return "Glsl: Attempt to use null shader";
	}
};

} } // namespace cinder::gl