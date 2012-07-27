/*
 Copyright (C) 2011-2012 Gabor Papp

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include <assert.h>

#include "cinder/app/App.h"
#include "cinder/ImageIo.h"
#include "cinder/CinderMath.h"
#include "cinder/Utilities.h"

#include "AssimpLoader.h"

using namespace std;
using namespace ci;

namespace mndl { namespace assimp {

static void fromAssimp( const aiMesh *aim, TriMesh *cim )
{
	// copy vertices
	for ( unsigned i = 0; i < aim->mNumVertices; ++i )
	{
		cim->appendVertex( fromAssimp( aim->mVertices[i] ) );
	}

	if( aim->HasNormals() )
	{
		for ( unsigned i = 0; i < aim->mNumVertices; ++i )
		{
			cim->appendNormal( fromAssimp( aim->mNormals[i] ) );
		}
	}

	// aiVector3D *	mTextureCoords [AI_MAX_NUMBER_OF_TEXTURECOORDS]
	// just one for now
	if ( aim->GetNumUVChannels() > 0 )
	{
		for ( unsigned i = 0; i < aim->mNumVertices; ++i )
		{
			cim->appendTexCoord( Vec2f( aim->mTextureCoords[0][i].x,
										aim->mTextureCoords[0][i].y ) );
		}
	}

	//aiColor4D *mColors [AI_MAX_NUMBER_OF_COLOR_SETS]
	if ( aim->GetNumColorChannels() > 0 )
	{
		for ( unsigned i = 0; i < aim->mNumVertices; ++i )
		{
			cim->appendColorRGBA( fromAssimp( aim->mColors[0][i] ) );
		}
	}

	for ( unsigned i = 0; i < aim->mNumFaces; ++i )
	{
		if ( aim->mFaces[i].mNumIndices > 3 )
		{
			throw AssimpLoaderExc( "non-triangular face found: model " +
					string( aim->mName.data ) + ", face #" +
					toString< unsigned >( i ) );
		}

		cim->appendTriangle( aim->mFaces[ i ].mIndices[ 0 ],
							 aim->mFaces[ i ].mIndices[ 1 ],
							 aim->mFaces[ i ].mIndices[ 2 ] );
	}
}

AssimpLoader::AssimpLoader( fs::path filename ) :
	mMaterialsEnabled( true ),
	mTexturesEnabled( true ),
	mSkinningEnabled( true ),
	mAnimationEnabled( false ),
	mFilePath( filename )
{
	// FIXME: aiProcessPreset_TargetRealtime_MaxQuality contains
	// aiProcess_Debone which is buggy in 3.0.1270
	unsigned flags = aiProcess_Triangulate |
					 aiProcess_FlipUVs |
					 aiProcessPreset_TargetRealtime_Quality |
					 aiProcess_FindInstances |
					 aiProcess_ValidateDataStructure |
					 aiProcess_OptimizeMeshes;

	mImporterRef = shared_ptr< Assimp::Importer >( new Assimp::Importer() );
	mImporterRef->SetPropertyInteger( AI_CONFIG_PP_SBP_REMOVE,
			aiPrimitiveType_LINE | aiPrimitiveType_POINT );
	mImporterRef->SetPropertyInteger( AI_CONFIG_PP_PTV_NORMALIZE, true );

	mScene = mImporterRef->ReadFile( filename.string(), flags );
	if ( !mScene )
		throw AssimpLoaderExc( mImporterRef->GetErrorString() );

	calculateDimensions();

	loadAllMeshes();
	mRootNode = loadNodes( mScene->mRootNode );
}

AssimpLoader::~AssimpLoader()
{
}

void AssimpLoader::calculateDimensions()
{
	Vec3f aMin, aMax;
	calculateBoundingBox( &aMin, &aMax );
	mBoundingBox = AxisAlignedBox3f( aMin, aMax );
}

void AssimpLoader::calculateBoundingBox( ci::Vec3f *min, ci::Vec3f *max )
{
	aiMatrix4x4 trafo;

	aiVector3D aiMin, aiMax;
	aiMin.x = aiMin.y = aiMin.z =  1e10f;
	aiMax.x = aiMax.y = aiMax.z = -1e10f;

	calculateBoundingBoxForNode( mScene->mRootNode, &aiMin, &aiMax, &trafo );
	*min = fromAssimp( aiMin );
	*max = fromAssimp( aiMax );
}

void AssimpLoader::calculateBoundingBoxForNode( const aiNode *nd, aiVector3D *min, aiVector3D *max, aiMatrix4x4 *trafo )
{
	aiMatrix4x4 prev;

	prev = *trafo;
	*trafo = *trafo * nd->mTransformation;

	for ( unsigned n = 0; n < nd->mNumMeshes; ++n )
	{
		const struct aiMesh *mesh = mScene->mMeshes[ nd->mMeshes[ n ] ];
		for ( unsigned t = 0; t < mesh->mNumVertices; ++t )
		{
			aiVector3D tmp = mesh->mVertices[ t ];
			tmp *= (*trafo);

			min->x = math<float>::min( min->x, tmp.x );
			min->y = math<float>::min( min->y, tmp.y );
			min->z = math<float>::min( min->z, tmp.z );
			max->x = math<float>::max( max->x, tmp.x );
			max->y = math<float>::max( max->y, tmp.y );
			max->z = math<float>::max( max->z, tmp.z );
		}
	}

	for ( unsigned n = 0; n < nd->mNumChildren; ++n )
	{
		calculateBoundingBoxForNode( nd->mChildren[n], min, max, trafo );
	}

	*trafo = prev;
}

AssimpNodeRef AssimpLoader::loadNodes( const aiNode *nd, AssimpNodeRef parentRef )
{
	AssimpNodeRef nodeRef = AssimpNodeRef( new AssimpNode() );
	nodeRef->setParent( parentRef );
	string nodeName = fromAssimp( nd->mName );
	nodeRef->setName( nodeName );
	mNodeMap[ nodeName] = nodeRef;
	mNodeNames.push_back( nodeName );

	// store transform
	aiVector3D scaling;
	aiQuaternion rotation;
	aiVector3D position;
	nd->mTransformation.Decompose( scaling, rotation, position );
	nodeRef->setScale( fromAssimp( scaling ) );
	nodeRef->setOrientation( fromAssimp( rotation ) );
	nodeRef->setPosition( fromAssimp( position ) );

	// meshes
	for ( unsigned i = 0; i < nd->mNumMeshes; ++i )
	{
		unsigned meshId = nd->mMeshes[ i ];
		if ( meshId >= mModelMeshes.size() )
			throw AssimpLoaderExc( "node " + nodeRef->getName() + " references mesh #" +
					toString< unsigned >( meshId ) + " from " +
					toString< size_t >( mModelMeshes.size() ) + " meshes." );
		nodeRef->mMeshes.push_back( mModelMeshes[ meshId ] );
	}

	// store the node with meshes for rendering
	if ( nd->mNumMeshes > 0 )
	{
		mMeshNodes.push_back( nodeRef );
	}

	// process all children
	for ( unsigned n = 0; n < nd->mNumChildren; ++n )
	{
		AssimpNodeRef childRef = loadNodes( nd->mChildren[ n ], nodeRef );
		nodeRef->addChild( childRef );
	}
	return nodeRef;
}

AssimpMeshHelperRef AssimpLoader::convertAiMesh( const aiMesh *mesh )
{
	// the current meshHelper we will be populating data into.
	AssimpMeshHelperRef meshHelperRef = AssimpMeshHelperRef( new AssimpMeshHelper() );

	meshHelperRef->mName = fromAssimp( mesh->mName );

	// Handle material info
	aiMaterial *mtl = mScene->mMaterials[ mesh->mMaterialIndex ];

	aiString name;
	mtl->Get( AI_MATKEY_NAME, name );
	app::console() << "material " << fromAssimp( name ) << endl;

	// Culling
	int twoSided;
	if ( ( AI_SUCCESS == mtl->Get( AI_MATKEY_TWOSIDED, twoSided ) ) && twoSided )
	{
		meshHelperRef->mTwoSided = true;
		meshHelperRef->mMaterial.setFace( GL_FRONT_AND_BACK );
		app::console() << " two sided" << endl;
	}
	else
	{
		meshHelperRef->mTwoSided = false;
		meshHelperRef->mMaterial.setFace( GL_FRONT );
	}

	aiColor4D dcolor, scolor, acolor, ecolor;
	if ( AI_SUCCESS == mtl->Get( AI_MATKEY_COLOR_DIFFUSE, dcolor ) )
	{
		meshHelperRef->mMaterial.setDiffuse( fromAssimp( dcolor ) );
		app::console() << " diffuse: " << fromAssimp( dcolor ) << endl;
	}

	if ( AI_SUCCESS == mtl->Get( AI_MATKEY_COLOR_SPECULAR, scolor ) )
	{
		meshHelperRef->mMaterial.setSpecular( fromAssimp( scolor ) );
		app::console() << " specular: " << fromAssimp( scolor ) << endl;
	}

	if ( AI_SUCCESS == mtl->Get( AI_MATKEY_COLOR_AMBIENT, acolor ) )
	{
		meshHelperRef->mMaterial.setAmbient( fromAssimp( acolor ) );
		app::console() << " ambient: " << fromAssimp( acolor ) << endl;
	}

	if ( AI_SUCCESS == mtl->Get( AI_MATKEY_COLOR_EMISSIVE, ecolor ) )
	{
		meshHelperRef->mMaterial.setEmission( fromAssimp( ecolor ) );
		app::console() << " emission: " << fromAssimp( ecolor ) << endl;
	}

	/*
	// FIXME: not sensible data, obj .mtl Ns 96.078431 -> 384.314
	float shininessStrength = 1;
	if ( AI_SUCCESS == mtl->Get( AI_MATKEY_SHININESS_STRENGTH, shininessStrength ) )
	{
		app::console() << "shininess strength: " << shininessStrength << endl;
	}
	float shininess;
	if ( AI_SUCCESS == mtl->Get( AI_MATKEY_SHININESS, shininess ) )
	{
		meshHelperRef->mMaterial.setShininess( shininess * shininessStrength );
		app::console() << "shininess: " << shininess * shininessStrength << "[" <<
			shininess << "]" << endl;
	}
	*/

	// TODO: handle blending
#if 0
	int blendMode;
	if(AI_SUCCESS == aiGetMaterialInteger(mtl, AI_MATKEY_BLEND_FUNC, &blendMode)){
		if(blendMode==aiBlendMode_Default){
			meshHelper.blendMode=OF_BLENDMODE_ALPHA;
		}else{
			meshHelper.blendMode=OF_BLENDMODE_ADD;
		}
	}
#endif

	// Load Textures
	int texIndex = 0;
	aiString texPath;

	// TODO: handle other aiTextureTypes
	if ( AI_SUCCESS == mtl->GetTexture( aiTextureType_DIFFUSE, texIndex, &texPath ) )
	{
		app::console() << " diffuse texture " << texPath.data;
		fs::path texFsPath( texPath.data );
		fs::path modelFolder = mFilePath.parent_path();
		fs::path relTexPath = texFsPath.parent_path();
		fs::path texFile = texFsPath.filename();
		fs::path realPath = modelFolder / relTexPath / texFile;
		app::console() << " [" << realPath.string() << "]" << endl;

		// texture wrap
		gl::Texture::Format format;
		int uwrap;
		if ( AI_SUCCESS == mtl->Get( AI_MATKEY_MAPPINGMODE_U_DIFFUSE( 0 ), uwrap ) )
		{
			switch ( uwrap )
			{
				case aiTextureMapMode_Wrap:
					format.setWrapS( GL_REPEAT );
					break;

				case aiTextureMapMode_Clamp:
					format.setWrapS( GL_CLAMP );
					break;

				case aiTextureMapMode_Decal:
					// If the texture coordinates for a pixel are outside [0...1]
					// the texture is not applied to that pixel.
					format.setWrapS( GL_CLAMP_TO_EDGE );
					break;

				case aiTextureMapMode_Mirror:
					// A texture coordinate u|v becomes u%1|v%1 if (u-(u%1))%2
					// is zero and 1-(u%1)|1-(v%1) otherwise.
					// TODO
					format.setWrapS( GL_REPEAT );
					break;
			}
		}
		int vwrap;
		if ( AI_SUCCESS == mtl->Get( AI_MATKEY_MAPPINGMODE_V_DIFFUSE( 0 ), vwrap ) )
		{
			switch ( vwrap )
			{
				case aiTextureMapMode_Wrap:
					format.setWrapT( GL_REPEAT );
					break;

				case aiTextureMapMode_Clamp:
					format.setWrapT( GL_CLAMP );
					break;

				case aiTextureMapMode_Decal:
					// If the texture coordinates for a pixel are outside [0...1]
					// the texture is not applied to that pixel.
					format.setWrapT( GL_CLAMP_TO_EDGE );
					break;

				case aiTextureMapMode_Mirror:
					// A texture coordinate u|v becomes u%1|v%1 if (u-(u%1))%2
					// is zero and 1-(u%1)|1-(v%1) otherwise.
					// TODO
					format.setWrapT( GL_REPEAT );
					break;
			}
		}

		//format.setWrap( GL_REPEAT, GL_REPEAT );
		// TODO: cache textures
		meshHelperRef->mTexture = gl::Texture( loadImage( realPath ), format );
	}

	meshHelperRef->mAiMesh = mesh;
	fromAssimp( mesh, &meshHelperRef->mCachedTriMesh );
	meshHelperRef->mValidCache = true;
	meshHelperRef->mAnimatedPos.resize( mesh->mNumVertices );
	if ( mesh->HasNormals() )
	{
		meshHelperRef->mAnimatedNorm.resize( mesh->mNumVertices );
	}

#if 0

	int usage;
	if(getAnimationCount()){
#ifndef TARGET_OPENGLES
		usage = GL_STREAM_DRAW;
#else
		usage = GL_DYNAMIC_DRAW;
#endif
	}else{
		usage = GL_STATIC_DRAW;

	}

	meshHelper.vbo.setVertexData(&mesh->mVertices[0].x,3,mesh->mNumVertices,usage,sizeof(aiVector3D));
	if(mesh->HasVertexColors(0)){
		meshHelper.vbo.setColorData(&mesh->mColors[0][0].r,mesh->mNumVertices,GL_STATIC_DRAW,sizeof(aiColor4D));
	}
	if(mesh->HasNormals()){
		meshHelper.vbo.setNormalData(&mesh->mNormals[0].x,mesh->mNumVertices,usage,sizeof(aiVector3D));
	}
	if (meshHelper.cachedMesh.hasTexCoords()){
		meshHelper.vbo.setTexCoordData(meshHelper.cachedMesh.getTexCoordsPointer()[0].getPtr(),mesh->mNumVertices,GL_STATIC_DRAW,sizeof(ofVec2f));
	}
#endif

	meshHelperRef->mIndices.resize( mesh->mNumFaces * 3 );
	unsigned j = 0;
	for ( unsigned x = 0; x < mesh->mNumFaces; ++x )
	{
		for ( unsigned a = 0; a < mesh->mFaces[x].mNumIndices; ++a)
		{
			meshHelperRef->mIndices[ j++ ] = mesh->mFaces[ x ].mIndices[ a ];
		}
	}

#if 0
	meshHelper.vbo.setIndexData(&meshHelper.indices[0],meshHelper.indices.size(),GL_STATIC_DRAW);
#endif
	return meshHelperRef;
}

void AssimpLoader::loadAllMeshes()
{
	for ( unsigned i = 0; i < mScene->mNumMeshes; ++i )
	{
		app::console() << "loading mesh " << i << "[" <<
			fromAssimp( mScene->mMeshes[ i ]->mName ) << "]" << endl;
		AssimpMeshHelperRef meshHelperRef = convertAiMesh( mScene->mMeshes[ i ] );
		mModelMeshes.push_back( meshHelperRef );
	}

#if 0
	animationTime = -1;
	setNormalizedTime(0);
#endif

	app::console() << "finished loading gl resources" << endl;
}

void AssimpLoader::updateAnimation()
{
	if ( mScene->mNumAnimations == 0 )
		return;

	// TODO: set this by caller, this for test only
	unsigned animationIndex = 0;
	float currentTime = math<float>::fmod( app::getElapsedSeconds(), 5.f ) / 5.f;

	const aiAnimation *mAnim = mScene->mAnimations[ animationIndex ];

	// calculate the transformations for each animation channel
	for( unsigned int a = 0; a < mAnim->mNumChannels; a++ )
	{
		const aiNodeAnim *channel = mAnim->mChannels[ a ];

		// TODO: change this to AssimpNode
		//aiNode *targetNode = mScene->mRootNode->FindNode( channel->mNodeName );
		AssimpNodeRef targetNode = getAssimpNode( fromAssimp( channel->mNodeName ) );

		// ******** Position *****
		aiVector3D presentPosition( 0, 0, 0 );
		if( channel->mNumPositionKeys > 0 )
		{
			// Look for present frame number. Search from last position if time is after the last time, else from beginning
			// Should be much quicker than always looking from start for the average use case.
			unsigned int frame = 0;// (currentTime >= lastAnimationTime) ? lastFramePositionIndex : 0;
			while( frame < channel->mNumPositionKeys - 1)
			{
				if( currentTime < channel->mPositionKeys[frame+1].mTime)
					break;
				frame++;
			}

			// interpolate between this frame's value and next frame's value
			unsigned int nextFrame = (frame + 1) % channel->mNumPositionKeys;
			const aiVectorKey& key = channel->mPositionKeys[frame];
			const aiVectorKey& nextKey = channel->mPositionKeys[nextFrame];
			double diffTime = nextKey.mTime - key.mTime;
			if( diffTime < 0.0)
				diffTime += mAnim->mDuration;
			if( diffTime > 0)
			{
				float factor = float( (currentTime - key.mTime) / diffTime);
				presentPosition = key.mValue + (nextKey.mValue - key.mValue) * factor;
			} else
			{
				presentPosition = key.mValue;
			}
		}

		// ******** Rotation *********
		aiQuaternion presentRotation( 1, 0, 0, 0);
		if( channel->mNumRotationKeys > 0)
		{
			unsigned int frame = 0;//(currentTime >= lastAnimationTime) ? lastFrameRotationIndex : 0;
			while( frame < channel->mNumRotationKeys - 1)
			{
				if( currentTime < channel->mRotationKeys[frame+1].mTime)
					break;
				frame++;
			}

			// interpolate between this frame's value and next frame's value
			unsigned int nextFrame = (frame + 1) % channel->mNumRotationKeys;
			const aiQuatKey& key = channel->mRotationKeys[frame];
			const aiQuatKey& nextKey = channel->mRotationKeys[nextFrame];
			double diffTime = nextKey.mTime - key.mTime;
			if( diffTime < 0.0)
				diffTime += mAnim->mDuration;
			if( diffTime > 0)
			{
				float factor = float( (currentTime - key.mTime) / diffTime);
				aiQuaternion::Interpolate( presentRotation, key.mValue, nextKey.mValue, factor);
			} else
			{
				presentRotation = key.mValue;
			}
		}

		// ******** Scaling **********
		aiVector3D presentScaling( 1, 1, 1);
		if( channel->mNumScalingKeys > 0)
		{
			unsigned int frame = 0;//(currentTime >= lastAnimationTime) ? lastFrameScaleIndex : 0;
			while( frame < channel->mNumScalingKeys - 1)
			{
				if( currentTime < channel->mScalingKeys[frame+1].mTime)
					break;
				frame++;
			}

			// TODO: (thom) interpolation maybe? This time maybe even logarithmic, not linear
			presentScaling = channel->mScalingKeys[frame].mValue;
		}

#if 0
		// build a transformation matrix from it
		//aiMatrix4x4& mat;// = mTransforms[a];
		aiMatrix4x4 mat = aiMatrix4x4( presentRotation.GetMatrix());
		mat.a1 *= presentScaling.x; mat.b1 *= presentScaling.x; mat.c1 *= presentScaling.x;
		mat.a2 *= presentScaling.y; mat.b2 *= presentScaling.y; mat.c2 *= presentScaling.y;
		mat.a3 *= presentScaling.z; mat.b3 *= presentScaling.z; mat.c3 *= presentScaling.z;
		mat.a4 = presentPosition.x; mat.b4 = presentPosition.y; mat.c4 = presentPosition.z;
		//mat.Transpose();

		// TODO: change this to AssimpNode
		targetNode->mTransformation = mat;
#endif
		targetNode->setOrientation( fromAssimp( presentRotation ) );
		targetNode->setScale( fromAssimp( presentScaling ) );
		targetNode->setPosition( fromAssimp( presentPosition ) );
	}
}

AssimpNodeRef AssimpLoader::getAssimpNode( const std::string &name )
{
	map< string, AssimpNodeRef >::iterator i = mNodeMap.find( name );
	if ( i != mNodeMap.end() )
		return i->second;
	else
		return AssimpNodeRef();
}

void AssimpLoader::setNodeOrientation( const string &name, const Quatf &rot )
{
	AssimpNodeRef node = getAssimpNode( name );
	if ( node )
		node->setOrientation( rot );
}

Quatf AssimpLoader::getNodeOrientation( const string &name )
{
	AssimpNodeRef node = getAssimpNode( name );
	if ( node )
		return node->getOrientation();
	else
		return Quatf();
}

void AssimpLoader::updateSkinning()
{
	vector< AssimpNodeRef >::const_iterator it = mMeshNodes.begin();
	for ( ; it != mMeshNodes.end(); ++it )
	{
		AssimpNodeRef nodeRef = *it;

		vector< AssimpMeshHelperRef >::const_iterator meshIt = nodeRef->mMeshes.begin();
		for ( ; meshIt != nodeRef->mMeshes.end(); ++meshIt )
		{
			AssimpMeshHelperRef meshHelperRef = *meshIt;

			// current mesh we are introspecting
			const aiMesh *mesh = meshHelperRef->mAiMesh;

			// calculate bone matrices
			std::vector< aiMatrix4x4 > boneMatrices( mesh->mNumBones );
			for ( unsigned a = 0; a < mesh->mNumBones; ++a )
			{
				const aiBone *bone = mesh->mBones[ a ];

				// find the corresponding node by again looking recursively through
				// the node hierarchy for the same name
				AssimpNodeRef nodeRef = getAssimpNode( fromAssimp( bone->mName ) );
				assert( nodeRef );
				boneMatrices[ a ] = toAssimp( nodeRef->getDerivedTransform() ) *
										bone->mOffsetMatrix;
#if 0
				// TODO: change this to AssimpNode
				aiNode *node = mScene->mRootNode->FindNode( bone->mName );

				// start with the mesh-to-bone matrix
				boneMatrices[ a ] = bone->mOffsetMatrix;
				// and now append all node transformations down the parent chain until
				// we're back at mesh coordinates again
				const aiNode *tempNode = node;
				while( tempNode )
				{
					// check your matrix multiplication order here!!!
					boneMatrices[ a ] = tempNode->mTransformation * boneMatrices[ a ];
					tempNode = tempNode->mParent;
				}
#endif
			}

			meshHelperRef->mValidCache = false;

			meshHelperRef->mAnimatedPos.assign( meshHelperRef->mAnimatedPos.size(),
					aiVector3D( 0, 0, 0 ) );
			if ( mesh->HasNormals() )
			{
				meshHelperRef->mAnimatedNorm.assign( meshHelperRef->mAnimatedNorm.size(),
						aiVector3D( 0, 0, 0 ) );
			}

			// loop through all vertex weights of all bones
			for ( unsigned a = 0; a < mesh->mNumBones; ++a )
			{
				const aiBone *bone = mesh->mBones[a];
				const aiMatrix4x4 &posTrafo = boneMatrices[ a ];

				for ( unsigned b = 0; b < bone->mNumWeights; ++b )
				{
					const aiVertexWeight &weight = bone->mWeights[ b ];
					size_t vertexId = weight.mVertexId;
					const aiVector3D& srcPos = mesh->mVertices[vertexId];

					meshHelperRef->mAnimatedPos[vertexId] += weight.mWeight * (posTrafo * srcPos);
				}

				if ( mesh->HasNormals() )
				{
					// 3x3 matrix, contains the bone matrix without the
					// translation, only with rotation and possibly scaling
					aiMatrix3x3 normTrafo = aiMatrix3x3( posTrafo );
					for ( size_t b = 0; b < bone->mNumWeights; ++b )
					{
						const aiVertexWeight& weight = bone->mWeights[b];
						size_t vertexId = weight.mVertexId;

						const aiVector3D& srcNorm = mesh->mNormals[vertexId];
						meshHelperRef->mAnimatedNorm[vertexId] += weight.mWeight * (normTrafo * srcNorm);
					}
				}
			}
		}
	}
}

void AssimpLoader::updateMeshes()
{
	vector< AssimpNodeRef >::iterator it = mMeshNodes.begin();
	for ( ; it != mMeshNodes.end(); ++it )
	{
		AssimpNodeRef nodeRef = *it;

		vector< AssimpMeshHelperRef >::iterator meshIt = nodeRef->mMeshes.begin();
		for ( ; meshIt != nodeRef->mMeshes.end(); ++meshIt )
		{
			AssimpMeshHelperRef meshHelperRef = *meshIt;

			if ( meshHelperRef->mValidCache )
				continue;

			if ( mSkinningEnabled )
			{
				// animated data
				std::vector< Vec3f > &vertices = meshHelperRef->mCachedTriMesh.getVertices();
				for( size_t v = 0; v < vertices.size(); ++v )
					vertices[v] = fromAssimp( meshHelperRef->mAnimatedPos[ v ] );

				std::vector< Vec3f > &normals = meshHelperRef->mCachedTriMesh.getNormals();
				for( size_t v = 0; v < normals.size(); ++v )
					normals[v] = fromAssimp( meshHelperRef->mAnimatedNorm[ v ] );
			}
			else
			{
				// original mesh data from assimp
				const aiMesh *mesh = meshHelperRef->mAiMesh;

				std::vector< Vec3f > &vertices = meshHelperRef->mCachedTriMesh.getVertices();
				for( size_t v = 0; v < vertices.size(); ++v )
					vertices[v] = fromAssimp( mesh->mVertices[ v ] );

				std::vector< Vec3f > &normals = meshHelperRef->mCachedTriMesh.getNormals();
				for( size_t v = 0; v < normals.size(); ++v )
					normals[v] = fromAssimp( mesh->mNormals[ v ] );
			}

			meshHelperRef->mValidCache = true;
		}
	}
}

void AssimpLoader::enableSkinning( bool enable /* = true */ )
{
	if ( mSkinningEnabled == enable )
		return;

	mSkinningEnabled = enable;
	// invalidate mesh cache
	vector< AssimpNodeRef >::const_iterator it = mMeshNodes.begin();
	for ( ; it != mMeshNodes.end(); ++it )
	{
		AssimpNodeRef nodeRef = *it;

		vector< AssimpMeshHelperRef >::const_iterator meshIt = nodeRef->mMeshes.begin();
		for ( ; meshIt != nodeRef->mMeshes.end(); ++meshIt )
		{
			AssimpMeshHelperRef meshHelperRef = *meshIt;
			meshHelperRef->mValidCache = false;
		}
	}
}

void AssimpLoader::update()
{
	if ( mAnimationEnabled )
		updateAnimation();

	if ( mSkinningEnabled )
		updateSkinning();

	updateMeshes();
}

void AssimpLoader::draw()
{
	glPushAttrib( GL_ALL_ATTRIB_BITS );
	glPushClientAttrib( GL_CLIENT_ALL_ATTRIB_BITS );
	gl::enable( GL_NORMALIZE );

	vector< AssimpNodeRef >::const_iterator it = mMeshNodes.begin();
	for ( ; it != mMeshNodes.end(); ++it )
	{
		AssimpNodeRef nodeRef = *it;

		vector< AssimpMeshHelperRef >::const_iterator meshIt = nodeRef->mMeshes.begin();
		for ( ; meshIt != nodeRef->mMeshes.end(); ++meshIt )
		{
			AssimpMeshHelperRef meshHelperRef = *meshIt;

			// Texture Binding
			if ( mTexturesEnabled && meshHelperRef->mTexture )
			{
				meshHelperRef->mTexture.enableAndBind();
			}

			if ( mMaterialsEnabled )
			{
				meshHelperRef->mMaterial.apply();
			}
			else
			{
				gl::color( meshHelperRef->mMaterial.getDiffuse());
			}

			// Culling
			if ( meshHelperRef->mTwoSided )
				gl::enable( GL_CULL_FACE );
			else
				gl::disable( GL_CULL_FACE );

			gl::draw( meshHelperRef->mCachedTriMesh );

			// Texture Binding
			if ( mTexturesEnabled && meshHelperRef->mTexture )
			{
				meshHelperRef->mTexture.unbind();
			}
		}
	}


	glPopClientAttrib();
	glPopAttrib();
}

} } // namespace mndl::assimp

