#ifndef FTB_ZOID_CLIENT_H
#define FTB_ZOID_CLIENT_H

#include <stdio.h>
#include <stdlib.h>
#include "zoid_api.h"

#include "ftb_def.h"

/* START-ZOID-SCANNER ID=10 INIT=ftb_zoid_client_init FINI=ftb_zoid_client_fini */

int ZOID_FTB_Connect( FTB_client_t *comp_info /* in:ptr */,
                   FTB_client_handle_t *client_handle /* out:ptr */);

int ZOID_FTB_Reg_throw( FTB_client_handle_t handle /* in:obj */,
                        const char *event /* in:str */);

int ZOID_FTB_Reg_catch_polling_event( FTB_client_handle_t handle /* in:obj */,
                                      const char *event /* in:str */);

int ZOID_FTB_Reg_catch_polling_mask( FTB_client_handle_t handle /* in:obj */,
                                     const FTB_event_t *event /* in:ptr */);

int ZOID_FTB_Reg_all_predefined_catch ( FTB_client_handle_t handle /* in:obj */);

int ZOID_FTB_Publish ( FTB_client_handle_t handle /* in:obj */,
                     const char *event /* in:str */,
                     FTB_event_data_t *datadetails /* in:ptr */,
                     char *error_msg /* out:str */);

int ZOID_FTB_Catch ( FTB_client_handle_t handle /* in:obj */,
                     FTB_event_t *event /* out:ptr */,
                     FTB_id_t *src /* out:ptr:nullok */);

int ZOID_FTB_Disconnect (FTB_client_handle_t handle /* in:obj */);

int ZOID_FTB_Abort (FTB_client_handle_t handle /* in:obj */);

int ZOID_FTB_Add_dynamic_tag(FTB_client_handle_t handle /* in:obj */,
                                FTB_tag_t tag /* in:obj */, 
                                const char *tag_data /* in:ptr */,
                                FTB_tag_len_t data_len /* in:obj */);

int ZOID_FTB_Remove_dynamic_tag (FTB_client_handle_t handle /* in:obj */,
                                FTB_tag_t tag /* in:obj */);
#endif /* FTB_ZOID_CLIENT_H */
