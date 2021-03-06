/**
 * @file test_api.h (V1)
 *  EM test entity API header (V1 wrapper)
 * Copyright (C) 2016 GM Corp. (http://www.grain-media.com)
 *
 * $Revision: 1.1 $
 * $Date: 2016/08/10 06:43:39 $
 *
 * ChangeLog:
 *  $Log: test_api.h,v $
 *  Revision 1.1  2016/08/10 06:43:39  ivan
 *  *** empty log message ***
 *
 */
#ifndef _TEST_API_H_
#define _TEST_API_H_

#include "common.h"

#define IOCTL_VA_TO_PA      0x5050
#define IOCTL_QUERY_ID_SET  0x5564
#define IOCTL_QUERY_STR     0x5565
#define IOCTL_TEST_CASES    0x5570

#define MAX_CFG         10
#define MAX_JOBS_SET    30
#define MAX_PARAMS      MAX_HUGE_PROPERTIES
#define MAX_NAME        50

//FOR VCAP ONLY
#define VCAP_VG_FD_ENGINE_VIMODE(x)                 ((x>>6)&0x7)
#define VCAP_VG_FD_ENGINE_VIMODE_SPLIT_IDX          ((0x3) << (6)) 
#define VCAP_VG_FD_MINOR_WITH_SPLIT_CH(minor,split_ch) ((minor) | ((split_ch) << 2))

struct video_param_t {
    char            name[MAX_NAME];
    int             vch;
    int             id;
    unsigned int    value;
    int             type;   //record its type
    unsigned int    reserved[4];
};


struct job_case_t {
    int enable;     //0:disable 1:enable
    int id; //record job id
    int chip;
    int engine;
    int minor;
    int status;     //return status
    int in_buf_type;   //TYPE_YUV422_2FRAMES...
    int out_buf_type;   //TYPE_YUV422_2FRAMES...
    unsigned int in_ddr_id[2];
    unsigned int in_mmap_va[2];
    unsigned int in_mmap_size[2];
    unsigned int out_ddr_id[2];
    unsigned int out_mmap_va[2];
    unsigned int out_mmap_size[2];
    struct video_properties_t in_prop;
    struct video_properties_t out_prop;
    void *priv;    //link to job structure
};

#define MAX_JOBS 100
struct test_case_t {
    int case_item;  //1:single job    2:burnin     3:multi jobs
    int type;       //TYPE_CAPTURE....
    int burnin_secs; //burnin seconds
    struct job_case_t job_cases[MAX_JOBS];
};


#endif
