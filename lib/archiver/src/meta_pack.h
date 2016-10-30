#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <QFile>
#include <QString>

#include <fs_tree.h>
#include <struct_serialization.pb.h>
#include "archiver_utils.h"

namespace apb = ArchiverUtils::protobufStructs;

namespace details{
    inline void init_stat(struct stat& stat, uid_t uid,
            gid_t gid, time_t atime, time_t mtime, mode_t mode) {
        stat.st_uid = uid;
        stat.st_gid = gid;
        stat.st_mtime = mtime;
        stat.st_atime = atime;
        stat.st_mode = mode;
    }

    inline void pack_inode(const inode *inode,
            apb::PBDirEntMetaData *packed)
    {
        packed->set_uid(inode->attrs.st_uid);
        packed->set_gid(inode->attrs.st_gid);
        packed->set_mtime(inode->attrs.st_mtime);
        packed->set_atime(inode->attrs.st_atime);
        packed->set_mode(inode->attrs.st_mode);

        if (inode->type == INODE_REG_FILE || inode->type == INODE_DIR)
        {
            if (inode->parent == NULL)
            {
                packed->set_name(ArchiverUtils::getDirentName(inode->name).toStdString());
            }
            else
            {
                packed->set_name(inode->name);
            }
        }
        else
        {
            std::cout << "Spec file: " << inode->name << std::endl;
        }
    }
}

inline void pack_regfile_inode(const regular_file_inode *inode,
        apb::PBDirEntMetaData *packed, std::uint64_t & contentFreePosition)
{
    details::pack_inode(&inode->inode, packed);
    packed->mutable_pbregfilemetadata()->set_contentsize(inode->inode.attrs.st_size);
    packed->mutable_pbregfilemetadata()->set_contentoffset(contentFreePosition);

    contentFreePosition += inode->inode.attrs.st_size;
}

inline void unpack_regfile_inode(const apb::PBDirEntMetaData *packed,
                                 regular_file_inode *inode, std::vector<struct inode*> & indexToInodePointer,
                                 std::uint64_t index)
{
    struct stat curStat;
    details::init_stat(curStat, packed->uid(), packed->gid(), packed->atime(), packed->mtime(), packed->mode());
    init_regular_file_inode_from_stat(inode, packed->name().c_str(),curStat,
                                      indexToInodePointer[packed->parentix()]);
    indexToInodePointer[index] = reinterpret_cast<struct inode*>(inode);
    inode->inode.user_data = reinterpret_cast<void*>(index);
}

inline void pack_dir_inode(const dir_inode *inode, apb::PBDirEntMetaData *packed,
        apb::PBArchiveMetaData *metaArchive,
        const std::uint64_t dirIndexInPBArchiveMetaData)
{
    details::pack_inode(&inode->inode, packed);
    for (size_t i = 0; i < inode->num_children; ++i)
    {
        inode->children[i]->user_data = (void*)((std::uint64_t)metaArchive->pbdirentmetadata_size());
        std::uint64_t childindex = metaArchive->pbdirentmetadata_size();
        metaArchive->add_pbdirentmetadata();
        metaArchive->mutable_pbdirentmetadata()->Mutable(childindex)->set_parentix(dirIndexInPBArchiveMetaData);
    }
}

inline void unpack_dir_inode(const apb::PBDirEntMetaData *packed,
     dir_inode * inode, std::vector<struct inode*> & indexToInodePointer, std::uint64_t numberDirChildren,
                             std::uint64_t index)
{
    struct stat curStat;
    details::init_stat(curStat ,packed->uid(), packed->gid(), packed->atime(), packed->mtime(), packed->mode());
    init_dir_inode_from_stat(inode, packed->name().c_str(), curStat,
                             indexToInodePointer[packed->parentix()], numberDirChildren);
    indexToInodePointer[index] = reinterpret_cast<struct inode*>(inode);
    inode->inode.user_data = reinterpret_cast<void*>(index);
}
