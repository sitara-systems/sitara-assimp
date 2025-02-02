/*
 Copyright (C) 2011-2012 Gabor Papp

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU Lesser General Public License as published
 by the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public License
 along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <vector>

//* 5.0
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
//*/

#include "cinder/Cinder.h"
#include "cinder/Color.h"
#include "cinder/TriMesh.h"
#include "cinder/Stream.h"
#include "cinder/AxisAlignedBox.h"
#include "cinder/gl/gl.h"

#include "Node.h"
#include "AssimpMesh.h"

namespace sitara {
	namespace assimp {
		inline ci::vec3 fromAssimp( const aiVector3D &v )
		{
			return ci::vec3( v.x, v.y, v.z );
		}

		inline aiVector3D toAssimp( const ci::vec3 &v )
		{
			return aiVector3D( v.x, v.y, v.z );
		}

		inline ci::quat fromAssimp( const aiQuaternion &q )
		{
			return ci::quat( q.w, q.x, q.y, q.z );
		}

		inline aiQuaternion toAssimp(const ci::quat& q) {
            return aiQuaternion(q.w, q.x, q.y, q.z);
        }

		inline ci::mat4 fromAssimp( const aiMatrix4x4 &m ) {
			return ci::mat4(m.a1, m.b1, m.c1, m.d1,
							m.a2, m.b2, m.c2, m.d2,
							m.a3, m.b3, m.c3, m.d3,
							m.a4, m.b4, m.c4, m.d4);
		}

		inline aiMatrix4x4 toAssimp( const ci::mat4 &m )
		{
			return aiMatrix4x4( m[0][0], m[0][1], m[0][2], m[0][3],
								m[1][0], m[1][1], m[1][2], m[1][3],
								m[2][0], m[2][1], m[2][2], m[2][3],
								m[3][0], m[3][1], m[3][2], m[3][3]);
		}

		inline ci::ColorAf fromAssimp( const aiColor4D &c )
		{
            return ci::ColorAf(c.r, c.g, c.b, c.a);
		}

		inline aiColor4D toAssimp( const ci::ColorAf &c )
		{
			return aiColor4D( c.r, c.g, c.b, c.a );
		}

		inline std::string fromAssimp( const aiString &s )
		{
			return std::string( s.data );
		}

		class AssimpLoaderExc : public std::exception
		{
			public:
				AssimpLoaderExc( const std::string &log ) throw()
				{
					strncpy_s( mMessage, log.c_str(), 512 );
				}

				virtual const char* what() const throw()
				{
					return mMessage;
				}

			private:
				char mMessage[ 513 ];
		};

		class AssimpLoader
		{
			public:
                static std::shared_ptr<AssimpLoader> create();
                static std::shared_ptr<AssimpLoader> create(std::filesystem::path& filename);
                void setFilename(const std::filesystem::path& filename);
				//! Used for async loading; CPU-only tasks and heavy processing
				void preloadModel();
				//! GL Shaders and conversion to ci::TriMesh
                void postloadModel();

				//! Updates model animation and skinning.
				void update();

                //! Draws mesh by index
				bool drawMesh(int index);
                //! Draws mesh by name
				bool drawMesh(const std::string& name);
				//! Draws all meshes in the model.
				void draw();

				//! Returns the bounding box of the static, not skinned mesh.
				ci::AxisAlignedBox getBoundingBox() const { return mBoundingBox; }

				//! Sets the orientation of this node via a quaternion.
				void setNodeOrientation( const std::string &name, const ci::quat &rot );
				//! Returns a quaternion representing the orientation of the node called \a name.
				ci::quat getNodeOrientation( const std::string &name );

				//! Returns the node called \a name.
				AssimpNodeRef getAssimpNode( const std::string &name );
				//! Returns the node called \a name.
				const AssimpNodeRef getAssimpNode( const std::string &name ) const;

				//! Returns the total number of meshes contained by the node called \a name.
				size_t getAssimpNodeNumMeshes( const std::string &name );
				//! Returns the \a n'th cinder::TriMesh contained by the node called \a name.
				ci::TriMeshRef getAssimpNodeMesh( const std::string &name, size_t n = 0 );
				//! Returns the \a n'th cinder::TriMesh contained by the node called \a name.
				const ci::TriMeshRef getAssimpNodeMesh( const std::string &name, size_t n = 0 ) const;

				//! Returns the texture of the \a n'th mesh in the node called \a name.
				ci::gl::Texture2dRef &getAssimpNodeTexture( const std::string &name, size_t n = 0 );
				//! Returns the texture of the \a n'th mesh in the node called \a name.
				const ci::gl::Texture2dRef &getAssimpNodeTexture( const std::string &name, size_t n = 0 ) const;

				//! Returns the material of the \a n'th mesh in the node called \a name.
				//sitara::assimp::Material& getAssimpNodeMaterial(const std::string& name, size_t n = 0);
				//! Returns the material of the \a n'th mesh in the node called \a name.
				//const sitara::assimp::Material& getAssimpNodeMaterial(const std::string& name, size_t n = 0) const;

				//! Returns all node names in the model in a std::vector as std::string's.
				const std::vector< std::string > &getNodeNames() { return mNodeNames; }

				//! Setter and getter for setting a custom shader program.  To use this program, you'll need to
				// call `enableCustomShader()`!
                void setCustomShader(ci::gl::GlslProgRef program) {
                    mCustomShaderProgram = program;
                }
                ci::gl::GlslProgRef getCustomShader() { return mCustomShaderProgram; }

				//! Enables the usage of a custom shader during draw; disables all other draw options!
				// If you enable this, you're on your own!
                void enableCustomShader(bool enable = true) { mCustomShaderEnabled = enable; }
				//! Disables the usage of a custom shader
                void disableCustomShader() { mCustomShaderEnabled = false; }

				//! Enables/disables the usage of materials during draw.
				void enableMaterials( bool enable = true ) { mMaterialsEnabled = enable; }
				//! Disables the usage of materials during draw.
				void disableMaterials() { mMaterialsEnabled = false; }

				//! Enables/disables the usage of textures during draw.
				void enableTextures( bool enable = true ) { mTexturesEnabled = enable; }
				//! Disables the usage of textures during draw.
				void disableTextures() { mTexturesEnabled = false; }

				//! Enables/disables skinning, when the model's bones distort the vertices.
				void enableSkinning( bool enable = true );
				//! Disables skinning, when the model's bones distort the vertices.
				void disableSkinning() { enableSkinning( false ); }

				//! Enables/disables animation.
				void enableAnimation( bool enable = true ) { mAnimationEnabled = enable; }
				//! Disables animation.
				void disableAnimation() { mAnimationEnabled = false; }

				//! Returns the total number of meshes in the model.
				size_t getNumMeshes() const { return mModelMeshes.size(); }

				//! Hides or shows the nth mesh of a model; this will make the mesh disappear when
				// calling draw() or drawMesh()
				void hideMesh(size_t n) { mModelMeshes[n]->mShowMesh = false; }
                void showMesh(size_t n) { mModelMeshes[n]->mShowMesh = true; }

				AssimpMeshRef getMesh(size_t n) { return mModelMeshes[n]; }
                const AssimpMeshRef getMesh(size_t n) const { return mModelMeshes[n]; }

				//! Returns the \a n'th mesh in the model.
				ci::TriMeshRef getTriMesh( size_t n ) { return mModelMeshes[ n ]->mCachedTriMesh; }
				//! Returns the \a n'th mesh in the model.
				const ci::TriMeshRef getTriMesh( size_t n ) const { return mModelMeshes[ n ]->mCachedTriMesh; }

				//! Returns the texture of the \a n'th mesh in the model.
				ci::gl::Texture2dRef getTexture( size_t n ) { return mModelMeshes[ n ]->mTexture; }
				//! Returns the texture of the \a n'th mesh in the model.
				const ci::gl::Texture2dRef getTexture( size_t n ) const { return mModelMeshes[ n ]->mTexture; }

				//! Returns the number of animations in the scene.
				size_t getNumAnimations() const;

				const std::vector<std::string>& getAnimationNames() const;

				const std::string& getAnimationName(size_t n) const;

				//! Sets the current animation index to \a n.
				void setAnimation( size_t n );

				//! Returns the duration of the \a n'th animation.
				double getAnimationDuration( size_t n ) const;

				//! Sets current animation time.
				void setTime( double t );

			private:
                //! Constructs the class and nothing more -- you'll need to manually set filename,
                //! preload, and postload.  See next constructor.
                AssimpLoader();

                //! Constructs and does the parsing of the file from \a filename.  Automatically sets
                //! filename, preloads, and postloads.
                AssimpLoader(const std::filesystem::path& filename);

				void loadAllMeshes();
				AssimpNodeRef loadNodes( const aiNode* nd, AssimpNodeRef parentRef = AssimpNodeRef() );
				AssimpMeshRef convertAiMesh( const aiMesh *mesh );
                void drawMesh(AssimpMeshRef mesh);

				void calculateDimensions();
				void calculateBoundingBox( ci::vec3 *min, ci::vec3 *max );
				void calculateBoundingBoxForNode( const aiNode *nd, aiVector3D *min, aiVector3D *max, aiMatrix4x4 *trafo );

				void updateAnimation( size_t animationIndex, double currentTime );
				void updateSkinning();
				void updateMeshes();

				std::shared_ptr< Assimp::Importer > mImporterRef; // mScene will be destroyed along with the Importer object
				ci::fs::path mFilePath; /// model path
				const aiScene *mScene;

				ci::AxisAlignedBox mBoundingBox;

				AssimpNodeRef mRootNode; /// root node of scene

				std::vector< AssimpNodeRef > mMeshNodes; /// nodes with meshes
				std::vector< AssimpMeshRef > mModelMeshes; /// all meshes

				std::vector< std::string > mNodeNames;
				std::map< std::string, AssimpNodeRef > mNodeMap;

				std::vector<std::string> mAnimationNames;

				bool mMaterialsEnabled;
				bool mTexturesEnabled;
				bool mSkinningEnabled;
				bool mAnimationEnabled;
                bool mCustomShaderEnabled;

				ci::gl::GlslProgRef mCustomShaderProgram;
                ci::gl::GlslProgRef mPhongShaderProgram;

				size_t mAnimationIndex;
				double mAnimationTime;
		};

		typedef std::shared_ptr<AssimpLoader> AssimpLoaderRef;
	}
}

