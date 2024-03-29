/*
 *
 *  The Sleuth Kit
 *
 *  Contact: Brian Carrier [carrier <at> sleuthkit [dot] org]
 *  Copyright (c) 2010-2012 Basis Technology Corporation. All Rights
 *  reserved.
 *
 *  This software is distributed under the Common Public License 1.0
 */


#ifndef _TSK_IMGDB_H
#define _TSK_IMGDB_H

#define IMGDB_SCHEMA_VERSION "1.0"

#include <string> // to get std::wstring
#include <list>
#include <vector>
#include "tsk3/libtsk.h"
#include "framework_i.h"
#include "Utilities/SectorRuns.h"
#include "Utilities/UnallocRun.h"
#include "TskBlackboardAttribute.h"
#include "TskBlackboard.h"
#include "TskBlackboardArtifact.h"

using namespace std;

class TskArtifactNames;
class TskAttributeNames;

typedef uint64_t artifact_t;


/**
 * Contains data from a volume/partition record in the database.
 */
struct TskVolumeInfoRecord
{
    uint64_t vol_id;
    TSK_DADDR_T sect_start;
    TSK_DADDR_T sect_len;
    std::string description;
    TSK_VS_PART_FLAG_ENUM flags;
};

/**
 * Contains data from a file system record in the database.
 */
struct TskFsInfoRecord
{
    uint64_t fs_id;
    TSK_OFF_T img_byte_offset;
    uint64_t vol_id;
    TSK_FS_TYPE_ENUM  fs_type;
    unsigned int block_size;
    TSK_DADDR_T block_count;
    TSK_INUM_T root_inum;
    TSK_INUM_T first_inum;
    TSK_INUM_T last_inum;
};

struct TskFileTypeRecord
{
    std::string suffix; // file extension, normalized to lowercase. If no extension, it is an empty string.
    std::string description; // descript of the file type.
    uint64_t count; // count of files with this extension.
};

struct TskModuleStatus;
struct TskBlackboardRecord;
struct TskUnallocImgStatusRecord;

/**
 * Contains data about the mapping of data in the unallocated chunks back
 * to their original location in the disk image.
 */
struct TskAllocUnallocMapRecord
{
    int vol_id;
    int unalloc_img_id;
    TSK_DADDR_T unalloc_img_sect_start;
    TSK_DADDR_T sect_len;
    TSK_DADDR_T orig_img_sect_start;
};

/**
 * contains data about the 'unused sectors', which did not have carvable data.
 */
struct TskUnusedSectorsRecord
{
    uint64_t fileId;
    TSK_DADDR_T sectStart;
    TSK_DADDR_T sectLen;
};

struct TskFileRecord;

/**
 * Interface for class that implments database storage for an image.
 * The database will be used to store information about the data
 * being analyzed. 
 * Can be registered with and retrieved from TskServices.
 */
class TSK_FRAMEWORK_API TskImgDB
{
public:
    enum FILE_TYPES
    {
        IMGDB_FILES_TYPE_FS = 0,
        IMGDB_FILES_TYPE_CARVED,
        IMGDB_FILES_TYPE_DERIVED,
        IMGDB_FILES_TYPE_UNUSED
    };

    enum FILE_STATUS
    {
        IMGDB_FILES_STATUS_CREATED = 0,
        IMGDB_FILES_STATUS_READY_FOR_ANALYSIS,
        IMGDB_FILES_STATUS_ANALYSIS_IN_PROGRESS,
        IMGDB_FILES_STATUS_ANALYSIS_COMPLETE,
        IMGDB_FILES_STATUS_ANALYSIS_FAILED,
        IMGDB_FILES_STATUS_ANALYSIS_SKIPPED
    };

    /**
     * Files have a 'known' status that is updated
     * with the use of hash databases. */
    enum KNOWN_STATUS
    {
        IMGDB_FILES_KNOWN = 0,  ///< 'Known', but cannot differentiate between good or bad.  NSRL, for example, identifies known, but does not assign a good or bad status. 
        IMGDB_FILES_KNOWN_GOOD,  ///< Known to be good / safely ignorable.
        IMGDB_FILES_KNOWN_BAD,  ///< Known to be bad or notable
        IMGDB_FILES_UNKNOWN     ///< Unknown files.  Perhaps because they haven't been analyzed yet or perhaps because they are user files that are not in a database.  All files start off in this state. 
    };

    /// Hash types supported by framework
    enum HASH_TYPE 
    {
        MD5 = 0,    ///< 128-bit MD5
        SHA1,       ///< 160-bit SHA1
        SHA2_256,   ///< 256-bit SHA2
        SHA2_512    ///< 512-bit SHA2
    };

    /// Data types that can be stored in blackboard
    enum VALUE_TYPE
    {
        BB_VALUE_TYPE_BYTE = 0, ///< Single byte
        BB_VALUE_TYPE_STRING,   ///< String 
        BB_VALUE_TYPE_INT32,    ///< 32-bit integer
        BB_VALUE_TYPE_INT64,    ///< 64-bit integer
        BB_VALUE_TYPE_DOUBLE    ///< double floating point
    };

    enum UNALLOC_IMG_STATUS
    {
        IMGDB_UNALLOC_IMG_STATUS_CREATED = 0,
        IMGDB_UNALLOC_IMG_STATUS_SCHEDULE_OK,
        IMGDB_UNALLOC_IMG_STATUS_SCHEDULE_ERR,
        IMGDB_UNALLOC_IMG_STATUS_CARVED_OK,
        IMGDB_UNALLOC_IMG_STATUS_CARVED_ERR,
        IMGDB_UNALLOC_IMG_STATUS_CARVED_NOT_NEEDED,
    };

    TskImgDB();
    virtual ~ TskImgDB();

    /**
     * Opens the database and creates the needed tables.
     * @returns 1 on error and 0 on success.
     */
    virtual int initialize() = 0;

    /**
     * Opens an existing database. Use initialize() to create
     * a new one.
     * @returns 1 on error and 0 on success.
     */
    virtual int open() = 0;

    /**
     * Close the database.
     * @returns 0 on success and 1 on failure.
     */
    virtual int close() = 0;

    virtual int begin() = 0;
    virtual int commit() = 0;

    virtual int addToolInfo(const char* name, const char* version) = 0;
    virtual int addImageInfo(int type, int sectorSize) = 0;
    virtual int addImageName(char const * imgName) = 0;
    virtual int addVolumeInfo(const TSK_VS_PART_INFO * vs_part) = 0;
    virtual int addFsInfo(int volId, int fsId, const TSK_FS_INFO * fs_info) = 0;
    virtual int addFsFileInfo(int fsId, const TSK_FS_FILE *fs_file, const char *name, int type, int idx, uint64_t & fileId, const char * path) = 0;
    virtual int addCarvedFileInfo(int vol_id, wchar_t * name, uint64_t size, uint64_t *runStarts, uint64_t *runLengths, int numRuns, uint64_t & fileId) = 0;
    virtual int addDerivedFileInfo(const std::string& name, const uint64_t parentId, 
                                        const bool isDirectory, const uint64_t size, const std::string& details,
                                        const int ctime, const int crtime, const int atime, const int mtime, uint64_t & fileId, std::string path) = 0;
    virtual int addFsBlockInfo(int fsID, uint64_t a_mFileId, int count, uint64_t blk_addr, uint64_t len) = 0;

    /**
     * Add information about how the unallocated images were created so that we can 
     later 
     * map where data was recovered from. This is typically used by CarvePrep and the results are 
     * used by CarveExtract via getUnallocRun(). 
     * @param a_volID Volume ID that the data was extracted from.
     * @param unallocImgID ID of the unallocated image that the sectors were copied into. 
     * @param unallocImgStart Sector offset of where in the unallocated image that t
     he run starts.
     * @param length Number of sectors that are in the run.
     * @param origImgStart Sector offset in the original image (relative to start of
        image) where the run starts  
     * @returns 1 on errror
     */
    virtual int addAllocUnallocMapInfo(int a_volID, int unallocImgID, uint64_t unallocImgStart, uint64_t length, uint64_t origImgStart) = 0;

    virtual int getSessionID() const = 0;
    virtual int getFileIds(char *a_fileName, uint64_t *a_outBuffer, int a_buffSize) const = 0;
    virtual int getNumFiles() const = 0;
    virtual int getMaxFileIdReadyForAnalysis(uint64_t a_lastFileId, uint64_t & maxFileId) const = 0;
    virtual int getMinFileIdReadyForAnalysis(uint64_t & minFileId) const = 0;
    virtual uint64_t getFileId(int fsId, uint64_t fs_file_id) const = 0;

    /**
     * Queries the blackboard for raw information about a specific file. 
     * @param fileId ID of file to lookup
     * @param fileRecord Location where data should be stored
     * @returns -1 on error and 0 on success.
     */
    virtual int getFileRecord(const uint64_t fileId, TskFileRecord& fileRecord) const = 0;
    virtual SectorRuns * getFileSectors(uint64_t fileId) const = 0;
    virtual std::vector<std::wstring> getImageNames() const = 0;
    virtual int getFileUniqueIdentifiers(uint64_t a_fileId, uint64_t &a_fsOffset, uint64_t &a_fsFileId, int &a_attrType, int &a_attrId) const = 0;
    virtual int getNumVolumes() const = 0;
    virtual int getImageInfo(int & type, int & sectorSize) const = 0;
    virtual int getVolumeInfo(std::list<TskVolumeInfoRecord> & volumeInfoList) const = 0;
    virtual int getFsInfo(std::list<TskFsInfoRecord> & fsInfoList) const = 0;
    virtual int getFileInfoSummary(std::list<TskFileTypeRecord>& fileTypeInfoList) const = 0;
    virtual int getFileInfoSummary(FILE_TYPES fileType, std::list<TskFileTypeRecord> & fileTypeInfoList) const = 0;
    /**
     * Return the known status of the file with the given id
     * @param fileId id of the file to get the status of
     * @returns KNOWN_STATUS or -1 on error
     */
    virtual KNOWN_STATUS getKnownStatus(const uint64_t fileId) const = 0;
    

    /**
     * Given an offset in an unallocated image that was created for carving, 
     * return information about where that data came from in the original image.
     * This is used to map where a carved file is located in the original image.
     * 
     * @param a_unalloc_img_id ID of the unallocated image that you want data about
     * @param a_file_offset Sector offset where file was found in the unallocated image
     * @return NULL on error or a run descriptor.  
     */
    virtual UnallocRun * getUnallocRun(int a_unalloc_img_id, int a_file_offset) const = 0; 

    /**
     * Returns a list of the sectors that are not used by files and that
     * are in unpartitioned space.  Typically this is used by CarvePrep.
     */
    virtual SectorRuns * getFreeSectors() const = 0;

    /**
     * update the status field in the database for a given file.
     * @param a_file_id File to update.
     * @param a_status Status flag to update to.
     * @returns 1 on error.
     */
    virtual int updateFileStatus(uint64_t a_file_id, FILE_STATUS a_status) = 0;

    /**
     * update the known status field in the database for a given file.
     * @param a_file_id File to update.
     * @param a_status Status flag to update to.
     * @returns 1 on error.
     */
    virtual int updateKnownStatus(uint64_t a_file_id, KNOWN_STATUS a_status) = 0;
	virtual bool dbExist() const = 0;

    // Get set of file ids that match the given condition (i.e. SQL where clause)
    virtual std::vector<uint64_t> getFileIds(std::string& condition) const = 0;
    virtual std::vector<const TskFileRecord> getFileRecords(std::string& condition) const = 0;

    // Get the number of files that match the given condition
    virtual int getFileCount(std::string& condition) const = 0;

    virtual std::map<uint64_t, std::string> getUniqueCarvedFiles(HASH_TYPE hashType) const = 0;
    virtual std::vector<uint64_t> getCarvedFileIds() const = 0;

    virtual std::vector<uint64_t> getUniqueFileIds(HASH_TYPE hashType) const = 0;
    virtual std::vector<uint64_t> getFileIds() const = 0;

    virtual int setHash(const uint64_t a_file_id, const TskImgDB::HASH_TYPE hashType, const std::string& hash) const = 0;
    virtual std::string getCfileName(const uint64_t a_file_id) const = 0;

    virtual int addModule(const std::string& name, const std::string& description, int & moduleId) = 0;
    virtual int setModuleStatus(uint64_t file_id, int module_id, int status) = 0;
    virtual int getModuleErrors(std::vector<TskModuleStatus> & moduleStatusList) const = 0;
    virtual std::string getFileName(uint64_t file_id) const = 0;

    /**
     * Used when a new unallocated image file is created for carving. 
     * @param unallocImgId [out] Stores the unique ID assigned to the image.
     * @returns -1 on error, 0 on success.
     */
    virtual int addUnallocImg(int & unallocImgId) = 0;

    virtual int setUnallocImgStatus(int unallocImgId, TskImgDB::UNALLOC_IMG_STATUS status) = 0;
    virtual TskImgDB::UNALLOC_IMG_STATUS getUnallocImgStatus(int unallocImgId) const = 0;
    virtual int getAllUnallocImgStatus(std::vector<TskUnallocImgStatusRecord> & unallocImgStatusList) const = 0;

    virtual int addUnusedSectors(int unallocImgId, std::vector<TskUnusedSectorsRecord> & unusedSectorsList) = 0;
    virtual int getUnusedSector(uint64_t fileId, TskUnusedSectorsRecord & unusedSectorsRecord) const = 0;

	// Quote and escape a string, the returned quoted string can be used as string literal in SQL statement.
	virtual std::string quote(const std::string str) const = 0;

    friend class TskDBBlackboard;

protected:
    // Blackboard methods.
    virtual TskBlackboardArtifact createBlackboardArtifact(uint64_t file_id, int artifactTypeID) = 0;
    virtual void addBlackboardAttribute(TskBlackboardAttribute attr) = 0;
    
    virtual string getArtifactTypeDisplayName(int artifactTypeID) = 0;
    virtual int getArtifactTypeID(string artifactTypeString) = 0;
    virtual string getArtifactTypeName(int artifactTypeID) = 0;
    virtual vector<TskBlackboardArtifact> getMatchingArtifacts(string whereClause) = 0;

    virtual void addArtifactType(int typeID, string artifactTypeName, string displayName) = 0;
    virtual void addAttributeType(int typeID, string attributeTypeName, string displayName)= 0;

    virtual string getAttributeTypeDisplayName(int attributeTypeID) = 0;
    virtual int getAttributeTypeID(string attributeTypeString) = 0;
    virtual string getAttributeTypeName(int attributeTypeID) = 0;
    virtual vector<TskBlackboardAttribute> getMatchingAttributes(string whereClause) = 0;
    TskBlackboardAttribute createAttribute(uint64_t artifactID, int attributeTypeID, uint64_t objectID, string moduleName, string context,
		TSK_BLACKBOARD_ATTRIBUTE_VALUE_TYPE valueType, int valueInt, uint64_t valueLong, double valueDouble, 
		string valueString, vector<unsigned char> valueBytes);
    TskBlackboardArtifact createArtifact(uint64_t artifactID, uint64_t objID, int artifactTypeID);
    virtual map<int, TskArtifactNames> getAllArtifactTypes();
    virtual map<int, TskAttributeNames> getAllAttributeTypes();
    virtual vector<int> findAttributeTypes(int artifactTypeId) = 0;

private:
    
};

/**
 * Contains data from a file record in the database.
 */
struct TskFileRecord
{
    uint64_t fileId;
    TskImgDB::FILE_TYPES typeId;
    std::string name;
    uint64_t parentFileId;
    TSK_FS_NAME_TYPE_ENUM dirType;
    TSK_FS_META_TYPE_ENUM metaType;
    TSK_FS_NAME_FLAG_ENUM dirFlags;
    TSK_FS_META_FLAG_ENUM metaFlags;
    TSK_OFF_T size;
    time_t ctime;
    time_t crtime;
    time_t atime;
    time_t mtime;
    TSK_FS_META_MODE_ENUM mode;
    TSK_UID_T uid;
    TSK_GID_T gid;
    TskImgDB::FILE_STATUS status;
    std::string md5;
    std::string sha1;
    std::string sha2_256;
    std::string sha2_512;
    std::string fullPath;
};

/**
 * Contains data about the module return status for a given file (as recorded in the database)
 */
struct TskModuleStatus
{
    uint64_t file_id;
    std::string module_name;
    int status;
};

/**
 * Contains data for a blackboard entry for a given file and artifact ID
 */
struct TskBlackboardRecord
{
    artifact_t artifactId;
    uint64_t fileId;    ///< File that this information pertains to.
    string attribute; ///< Name / type of the data being stored. Standard attribute names are defined in TskBlackboard
    string source;  ///< Name of the module that added this data
    string context; ///< Optional string that provides more context about the data.  For example, it may have "Last Printed" if the entry is a DATETIME entry about when a document was last printed.
    TskImgDB::VALUE_TYPE valueType; ///< Type of data being stored
    int32_t valueInt32;
    int64_t valueInt64;
    string valueString;
    double valueDouble;
    vector<unsigned char> valueByte;

    TskBlackboardRecord(artifact_t a_artifactId, uint64_t a_fileId, string a_attribute, string a_source, string a_context)
        : artifactId(a_artifactId), fileId(a_fileId), attribute(a_attribute), source(a_source), context(a_context)
    {
    }
    TskBlackboardRecord() {}
};

/**
 * Contains data about the current status for an unallocated chunk of data.
 */
struct TskUnallocImgStatusRecord
{
    int unallocImgId;
    TskImgDB::UNALLOC_IMG_STATUS status;
};


#endif
