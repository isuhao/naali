// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "DebugOperatorNew.h"

#include "OgreSkeletonAsset.h"
#include "OgreRenderingModule.h"
#include "AssetAPI.h"
#include "AssetCache.h"
#include "LoggingFunctions.h"

#include <QFile>
#include <QFileInfo>

#include "MemoryLeakCheck.h"

using namespace OgreRenderer;

OgreSkeletonAsset::~OgreSkeletonAsset()
{
    Unload();
}

bool OgreSkeletonAsset::DeserializeFromData(const u8 *data_, size_t numBytes, bool allowAsynchronous)
{
    if (!data_)
    {
        LogError("OgreSkeletonAsset::DeserializeFromData: Null source asset data pointer");
        return false;
    }
    if (numBytes == 0)
    {
        LogError("OgreSkeletonAsset::DeserializeFromData: Zero sized skeleton asset");
        return false;
    }
    
    /// Force an unload of this data first.
    Unload();

    if (assetAPI->GetFramework()->HasCommandLineParameter("--noAsyncAssetLoad") ||
        assetAPI->GetFramework()->HasCommandLineParameter("--no_async_asset_load")) /**< @todo Remove support for the deprecated underscore version at some point. */
    {
        allowAsynchronous = false;
    }

    // Asynchronous loading
    // 1. AssetAPI allows a asynch load. This is false when called from LoadFromFile(), LoadFromCache() etc.
    // 2. We have a rendering window for Ogre as Ogre::ResourceBackgroundQueue does not work otherwise. Its not properly initialized without a rendering window.
    // 3. The Ogre we are building against has thread support.
    if (allowAsynchronous && assetAPI->Cache() && !assetAPI->IsHeadless() && (OGRE_THREAD_SUPPORT != 0))
    {
        // We can only do threaded loading from disk, and not any disk location but only from asset cache.
        // local:// refs will return empty string here and those will fall back to the non-threaded loading.
        // Do not change this to do DiskCache() as that directory for local:// refs will not be a known resource location for ogre.
        QString cacheDiskSource = assetAPI->GetAssetCache()->FindInCache(Name());
        if (!cacheDiskSource.isEmpty())
        {
            QFileInfo fileInfo(cacheDiskSource);
            std::string sanitatedAssetRef = fileInfo.fileName().toStdString(); 
            loadTicket_ = Ogre::ResourceBackgroundQueue::getSingleton().load(Ogre::SkeletonManager::getSingleton().getResourceType(),
                              sanitatedAssetRef, OgreRenderer::OgreRenderingModule::CACHE_RESOURCE_GROUP, false, 0, 0, this);
            return true;
        }
    }

    // Synchronous loading
    try
    {
        if (ogreSkeleton.isNull())
        {
            ogreSkeleton = Ogre::SkeletonManager::getSingleton().create(
                AssetAPI::SanitateAssetRef(this->Name().toStdString()), Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

            if (ogreSkeleton.isNull())
            {
                LogError("OgreSkeletonAsset::DeserializeFromData: Failed to create skeleton " + this->Name());
                return false; 
            }
        }

        std::vector<u8> tempData(data_, data_ + numBytes);
#include "DisableMemoryLeakCheck.h"
        Ogre::DataStreamPtr stream(new Ogre::MemoryDataStream(&tempData[0], numBytes, false));
#include "EnableMemoryLeakCheck.h"
        Ogre::SkeletonSerializer serializer;
        serializer.importSkeleton(stream, ogreSkeleton.getPointer());
    }
    catch(Ogre::Exception &e)
    {
        LogError("OgreSkeletonAsset::DeserializeFromData: Failed to create skeleton " + this->Name() + ": " + QString(e.what()));
        Unload();
        return false;
    }

    internal_name_ = AssetAPI::SanitateAssetRef(this->Name().toStdString());
    //LogDebug("Ogre skeleton " + this->Name().toStdString() + " created");

    // We did a synchronous load, must call AssetLoadCompleted here.
    assetAPI->AssetLoadCompleted(Name());
    return true;
}

void OgreSkeletonAsset::operationCompleted(Ogre::BackgroundProcessTicket ticket, const Ogre::BackgroundProcessResult &result)
{
    if (ticket != loadTicket_)
        return;

    // Reset to 0 to mark the asynch request is not active anymore. Aborted in Unload() if it is.
    loadTicket_ = 0;
    
    const QString assetRef = Name();
    internal_name_ = AssetAPI::SanitateAssetRef(assetRef).toStdString();
    if (!result.error)
    {
        ogreSkeleton = Ogre::SkeletonManager::getSingleton().getByName(internal_name_, OgreRenderer::OgreRenderingModule::CACHE_RESOURCE_GROUP);
        if (!ogreSkeleton.isNull())
        {
            assetAPI->AssetLoadCompleted(assetRef);
            return;
        }
        else
            LogError("OgreSkeletonAsset asynch load: Could not get Skeleton from SkeletonManager after threaded loading: " + assetRef);
    }
    else
        LogError("OgreSkeletonAsset asynch load: Ogre failed to do threaded loading: " + result.message);

    DoUnload();
    assetAPI->AssetLoadFailed(assetRef);
}

bool OgreSkeletonAsset::SerializeTo(std::vector<u8> &data, const QString &serializationParameters) const
{
    if (ogreSkeleton.isNull())
    {
        LogWarning("Tried to export non-existing Ogre skeleton " + Name() + ".");
        return false;
    }
    try
    {
        Ogre::SkeletonSerializer serializer;
        QString tempFilename = assetAPI->GenerateTemporaryNonexistingAssetFilename(Name());
        // Ogre has a limitation of not supporting serialization to a file in memory, so serialize directly to a temporary file,
        // load it up and delete the temporary file.
        serializer.exportSkeleton(ogreSkeleton.get(), tempFilename.toStdString());
        bool success = LoadFileToVector(tempFilename.toStdString().c_str(), data);
        QFile::remove(tempFilename); // Delete the temporary file we used for serialization.
        if (!success)
            return false;
    } catch(std::exception &e)
    {
        LogError("Failed to export Ogre skeleton " + Name() + ":");
        if (e.what())
            LogError(e.what());
        return false;
    }
    return true;
}

void OgreSkeletonAsset::DoUnload()
{
    // If a ongoing asynchronous asset load requested has been made to ogre, we need to abort it.
    // Otherwise Ogre will crash to our raw pointer that was passed if we get deleted. A ongoing ticket id cannot be 0.
    if (loadTicket_ != 0)
    {
        Ogre::ResourceBackgroundQueue::getSingleton().abortRequest(loadTicket_);
        loadTicket_ = 0;
    }
        
    if (ogreSkeleton.isNull())
        return;

    std::string skeleton_name = ogreSkeleton->getName();
    ogreSkeleton.setNull();

    try
    {
        Ogre::SkeletonManager::getSingleton().remove(skeleton_name);
    }
    catch(...) {}
}

bool OgreSkeletonAsset::IsLoaded() const
{
    return ogreSkeleton.get() != 0;
}
