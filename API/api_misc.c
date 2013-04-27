#include "fs_int.h"
#include "fs_api.h"
#include "fs_clib.h"
#include "def.h"
#include "fs_fat.h"


/*********************************************************************
*
*             #define constants
*
**********************************************************************
*/

#define FS_VALID_MODE_NUM     (sizeof(_FS_valid_modes) / sizeof(_FS_mode_type))
/*********************************************************************
*
*             Local data types
*
**********************************************************************
*/

typedef struct {
  S8  *mode;
  U8  mode_r;     /* mode READ                    */
  U8  mode_w;     /* mode WRITE                   */
  U8  mode_a;     /* mode APPEND                  */
  U8  mode_c;     /* mode CREATE                  */
  U8  mode_b;     /* mode BINARY                  */
} _FS_mode_type;


/*********************************************************************
*
*             Local variables        
*
**********************************************************************
*/

static const _FS_mode_type _FS_valid_modes[] = {
  /*       READ  WRITE  APPEND  CREATE  BINARY */
  { "r"   ,  1,    0,     0,       0,     0 },
  { "w"   ,  0,    1,     0,       1,     0 },
  { "a"   ,  0,    1,     1,       1,     0 },
  { "rb"  ,  1,    0,     0,       0,     1 },
  { "wb"  ,  0,    1,     0,       1,     1 },
  { "ab"  ,  0,    1,     1,       1,     1 },
  { "r+"  ,  1,    1,     0,       0,     0 },
  { "w+"  ,  1,    1,     0,       1,     0 },
  { "a+"  ,  1,    1,     1,       1,     0 },
  { "r+b" ,  1,    1,     0,       0,     1 },
  { "rb+" ,  1,    1,     0,       0,     1 },
  { "w+b" ,  1,    1,     0,       1,     1 },
  { "wb+" ,  1,    1,     0,       1,     1 },
  { "a+b" ,  1,    1,     1,       1,     1 },
  { "ab+" ,  1,    1,     1,       1,     1 }
};

static const unsigned int _FS_maxopen = FS_MAXOPEN;
static FS_FILE            _FS_filehandle[FS_MAXOPEN];


/*********************************************************************
*
*             FS_FOpen
*
  Description:
  API function. Open an existing file or create a new one.

  Parameters:
  pFileName   - Fully qualified file name. 
  pMode       - Mode for opening the file.
  
  Return value:
  ==0         - Unable to open the file.
  !=0         - Address of an FS_FILE data structure.
*/

FS_FILE *FS_FOpen(const S8 *pFileName, const S8 *pMode) {
  S8 *s;
  FS_FILE *handle;
  U32 i;
  S32 j;
  S32 c;
  s=(S8 *)pFileName;
  /* Find correct FSL  (device:unit:name) */
  if (FS__pDevInfo[0].fs_ptr->fsl_fopen) {
    /*  Find next free entry in _FS_filehandle */
    i = 0;
    while (1) {
      if (i >= _FS_maxopen) {
        break;  /* No free entry found. */
      }
      if (!_FS_filehandle[i].inuse) {
        break;  /* Unused entry found */
      }
      i++;
    }
    if (i < _FS_maxopen) {
      /*
         Check for valid mode string and set flags in file
         handle
      */
      j = 0;
      while (1) {
        if (j >= FS_VALID_MODE_NUM) {
          break;  /* Not in list of valid modes */
        }
        c = FS__CLIB_strcmp(pMode, _FS_valid_modes[j].mode);
        if (c == 0) {
          break;  /* Mode found in list */
        }
        j++;
      }
      if (j < FS_VALID_MODE_NUM) {
        /* Set mode flags according to the mode string */
        _FS_filehandle[i].mode_r = _FS_valid_modes[j].mode_r;
        _FS_filehandle[i].mode_w = _FS_valid_modes[j].mode_w;
        _FS_filehandle[i].mode_a = _FS_valid_modes[j].mode_a;
        _FS_filehandle[i].mode_c = _FS_valid_modes[j].mode_c;
        _FS_filehandle[i].mode_b = _FS_valid_modes[j].mode_b;
      }
      else {
        return 0;
      }
      /* Execute the FSL function */
      handle = (FS__pDevInfo[0].fs_ptr->fsl_fopen)(s, pMode, &_FS_filehandle[i]);
      return handle;
    }
  }
  return 0;
}


/*********************************************************************
*
*             FS_FClose
*
  Description:
  API function. Close a file referred by pFile.

  Parameters:
  pFile       - Pointer to a FS_FILE data structure. 
  
  Return value:
  None.
*/

void FS_FClose(FS_FILE *pFile) {
  if (!pFile) {
    return;  /* No pointer to a FS_FILE structure */
  }
  if (!pFile->inuse) {
    return;
  }
    if (FS__pDevInfo[0].fs_ptr->fsl_fclose) {
      /* Execute the FSL function */
      (FS__pDevInfo[0].fs_ptr->fsl_fclose)(pFile);
    }
}

/*********************************************************************
*
*             FS_Remove
*
  Description:
  API function. Remove a file.
  There is no real 'delete' function in the FSL, but the FSL's 'open'
  function can delete a file. 

  Parameters:
  pFileName   - Fully qualified file name. 
  
  Return value:
  ==0         - File has been removed.
  ==-1        - An error has occured.
*/

S32 FS_Remove(const S8 *pFileName) {
  S8 *s;
  S32 i;
  S32 x;

  /* Find correct FSL  (device:unit:name) */
  s=(S8 *)pFileName;
  if (FS__pDevInfo[0].fs_ptr->fsl_fopen) {
    /*  Find next free entry in _FS_filehandle */
    i = 0;
    while (1) {
      if (i >= _FS_maxopen) {
        break;  /* No free file handle found */
      }
      if (!_FS_filehandle[i].inuse) {
        break;  /* Free file handle found */
      }
      i++;
    }
    if (i < _FS_maxopen) {
      /* Set file open mode to write & truncate */
      _FS_filehandle[i].mode_r = 0;
      _FS_filehandle[i].mode_w = 1;
      _FS_filehandle[i].mode_a = 0;
      _FS_filehandle[i].mode_c = 0;
      _FS_filehandle[i].mode_b = 0;
      /* 
         Call the FSL function 'open' with the parameter 'del' to indicate,
         that we want to delete the file.
      */
      x=(S32)((FS__pDevInfo[0].fs_ptr->fsl_fopen)(s, "del", &_FS_filehandle[i]));
      return x;
    }
  }
  return -1;
}


/*********************************************************************
*
*             FS_FSeek
*
  Description:
  API function. Set current position of a file pointer.
  FS_fseek does not support to position the fp behind end of a file. 

  Parameters:
  pFile       - Pointer to a FS_FILE data structure.
  Offset      - Offset for setting the file pointer position.
  Whence      - Mode for positioning the file pointer.
  
  Return value:
  ==0         - File pointer has been positioned according to the
                parameters.
  ==-1        - An error has occured.
*/

S32 FS_FSeek(FS_FILE *pFile, U32 Offset, S32 Whence) {
  S32 value;
  
  if (!pFile) {
    return -1;
  }
  pFile->CurClust  = 0;            /* Invalidate current cluster */
  if (Whence == FS_SEEK_SET) {
    if (Offset <= pFile->FileSize) {
      pFile->Offset = Offset;
    }
    else {
      /* New position would be behind EOF */
      return -1;
    }
  }
  else if (Whence == FS_SEEK_CUR) {
    value = pFile->Offset + Offset;
    if (value <= pFile->FileSize) {
      pFile->Offset += Offset;
    }
    else {
      /* New position would be behind EOF */
      return -1;
    }
  }
  else if (Whence == FS_SEEK_END) {
    /* The file system does not support this */
    return -1;
  }
  else {
    /* Parameter 'Whence' is invalid */
    return -1;
  }
  return 0;
}


/*********************************************************************
*
*             FS_FTell
*
  Description:
  API function. Return position of a file pointer.

  Parameters:
  pFile       - Pointer to a FS_FILE data structure.
  
  Return value:
  >=0         - Current position of the file pointer.
  ==-1        - An error has occured.
*/

S32 FS_FTell(FS_FILE *pFile) {
  if (!pFile) {
    return -1;
  }
  return pFile->Offset;
}

/*********************************************************************
*
*             FS_Init
*
  Description:
  API function. Start the file system.

  Parameters:
  None.
  
  Return value:
  ==0         - File system has been started.
  !=0         - An error has occured.
*/

void  FS_Init(void) {
 int i;
 for(i=0;i<FS_MAXOPEN;i++)
     _FS_filehandle[i].inuse=0;
  
  FS__fat_block_init(); /* Init the FAT layers memory pool */
}
/*********************************************************************
*
*             FS_Eixt
*
  before the systerm exit,we should close all the file
*
*
*
*
*/
void FS_Exit(void){
  S32 i;
  i = 0;
  while (1) {
    if (i >= _FS_maxopen) {
      break;  /* No free entry found. */
    }
    if (_FS_filehandle[i].inuse) {
      FS_FClose(&_FS_filehandle[i]);  /* Unused entry found */
    }
    i++;
  }
}


FS_FILE *FS_Get_Fhandle(S32 file_id){

  if(file_id>=0&&file_id<FS_MAXOPEN){
	if(_FS_filehandle[file_id].inuse==0){
	  return 0;
    }
	return (&_FS_filehandle[file_id]);
  }
}


