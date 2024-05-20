
#include "ops.h"
#include "logging.h"

// see more info by `man access`
int op_access(const char *path, int mask) {
    DEBUG("access path %s with mask %o", path, mask);
    // // check if has permissions
    // struct fuse_context *cntx=fuse_get_context();
    // uid_t uid=cntx->uid;
    // gid_t gid=cntx->gid;
    
    // if(mask==F_OK){
    //     return 0;
    // }
    // if(mask!=F_OK && uid!=cntx->uid && gid!=cntx->gid){
    //     return -EACCES;
    // }
    return 0;
}