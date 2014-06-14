#ifndef _DEBUG_H_
#define _DEBUG_H_

void dmp_fr (t_mpeg1_header *hdr);
void dmp_si (t_mpeg1_header *hdr, t_mpeg1_side_info *si);
void dmp_scf (t_mpeg1_side_info *si, t_mpeg1_main_data *md, int gr, int ch);
void dmp_huff (t_mpeg1_main_data *md, int gr, int ch);
void dmp_samples (t_mpeg1_main_data *md, int gr, int ch, int type);

#endif
