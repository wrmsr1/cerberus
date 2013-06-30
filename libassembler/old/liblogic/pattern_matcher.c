/*
 * ͷ�ļ�
 */
#include "pattern_matcher.h"
#include <math.h>

/*
 * �꺯��
 */
/* x�Ƿ�С��y */
#define _mix_(x, y) (((x) < (y)) ? (x) : (y))
/* x�Ƿ����y */
#define _max_(x, y) (((x) > (y)) ? (x) : (y))
/* ��ϣֵ���� */
#define _hash16_(p) ((word_t)(((*p)<<8) | *(p+1)))

/*
 * ��������
 */
/* BM�㷨���ɺ�׺λ�� */
static void bm_make_suffixes(const byte_t *, dword_t, dword_t *);
/* �Ա������ڴ泤�� */
static dword_t bcompare(const byte_t *, dword_t, const byte_t *, dword_t);
/* �ԱȺ���,����qsort�㷨 */
static dword_t qsortcmp(const byte_t *, const byte_t *);
/* WMģʽ���� */
static bool_t analyze_wm_pattern(PWM);
/* WM�㷨Ԥ����,����ģʽ�����ϣ�� */
static bool_t build_hashtbl_patgrps(PWM);
/* WM�㷨���ֽڹ���� */
static void wm_badchar_tbl(PWM);
/* WM�㷨���ֹ���� */
static bool_t wm_badword_tbl(PWM);
/* WM����������鵥�ֽ� */
dword_t wm_search_normal(PWM);
/* WM��������ʹ�û��ֽڱ� */
dword_t wm_searchex_bc(PWM);
/* WM��������ʹ�û��ֱ� */
dword_t wm_searchex_bw(PWM);
/* WM������ϣ����ƥ�� */
dword_t wm_group_matcher(PWM, dword_t, const byte_t *, const byte_t *, dword_t, wm_get1);
/* WMԤ���� */
bool_t perp_wm_patterns(PWM);

/*
 * ��������
 */
/*
 * ����:����
 * ��ʼ����:2009-7-7
 * �������:2009-7-7
 * ����:
 *    ptn:ģʽ.
 *    ptnlen:ģʽ����.
 * ����ֵ:
 *	  �ɹ�:����skip����.
 *    ʧ��:NULL.
 * ˵��:
 *    BM�㷨���ɻ��ַ�����.
 */
dword_t *bm_make_bc(const byte_t *ptn, dword_t ptnlen) {
	dword_t i;
	dword_t *bc = NULL;
	bc = (dword_t *)logic_malloc(sizeof(dword_t) * 0x100);
	if (!bc) return NULL;
	/* Ĭ���ƶ�λ��Ϊģʽ���� */
	for (i = 0; i < 0x100; i++)
		bc[i] = ptnlen;
	/* 
	 * ��ÿ���ַ����ص�λ���ƶ�Ϊ��ĩ�����뵱ǰ�ַ������Ĳ�ֵ
	 * �����������㷨����ȷ��
	 */
	for (i = 0; i < ptnlen - 1; i++)
		bc[ptn[i]] = ptnlen - 1 - i;
	return bc;
}/* end bm_make_bc */

/*
 * ����:����
 * ��ʼ����:2009-7-12
 * �������:2009-7-12
 * ����:
 *    ptn:ģʽָ��.
 *    ptnlen:ģʽ����.
 *    suffixes:����Ϊptnlen + 1������ָ��.
 * ����ֵ:
 *	  �ɹ�:suffixes����ָ��.
 *    ʧ��:NULL.
 * ˵��:
 *    BM�㷨���ɺ�׺����.
 */
static void bm_make_suffixes(const byte_t *ptn, dword_t ptnlen, dword_t *suffixes) {
	sdword_t f = 0, g = 0, i = 0;
	suffixes[ptnlen - 1] = ptnlen;
	g = ptnlen - 1;
	for (i = ptnlen - 2; i >= 0; i--) {
		if (i > g && suffixes[i + ptnlen - 1 - f] < i - g)
			suffixes[i] = suffixes[i + ptnlen - 1 - f];
		else {
			if (i < g) g = i;
			f = i;
			while(g >= 0 && ptn[g] == ptn[g + ptnlen - 1 - f]) g--;
			suffixes[i] = f - g;
		}/* end else */
	}/* end for */
}/* end bm_make_suffixes */


/*
 * ����:����
 * ��ʼ����:2009-7-12
 * �������:2009-7-12
 * ����:
 *    ptn:ģʽָ��.
 *    ptnlen:ģʽ����.
 * ����ֵ:
 *	  �ɹ�:�ú�׺����ָ��.
 *    ʧ��:NULL.
 * ˵��:
 *    BM�㷨���ɺú�׺����.
 */
dword_t *bm_make_gs(const byte_t *ptn, dword_t ptnlen) {
	sdword_t i = 0, j = 0;
	dword_t *suffixes = NULL, *gs = NULL;
	suffixes = (dword_t *)logic_malloc(sizeof(dword_t) * (ptnlen + 1));
	if (!suffixes) return NULL;
	gs = (dword_t *)logic_malloc(sizeof(dword_t) * (ptnlen + 1));
	if (!gs) {
		logic_free(suffixes);
		return NULL;
	}
	/* ���ɺ�׺ */
	bm_make_suffixes(ptn, ptnlen, suffixes);
	for (i = 0; i < ptnlen; i++) gs[i] = ptnlen;
	j = 0;
	for (i = ptnlen - 1; i >= 0; i--)
		if (suffixes[i] == i + 1)
			for (; j < ptnlen - 1 - i; j++)
				if (gs[j] == ptnlen) gs[j] = ptnlen - 1 - i;

	for (i = 0; i <= ptnlen - 2; i++)
		gs[ptnlen - 1 - suffixes[i]] = ptnlen - 1 - i;
	logic_free(suffixes);
	return gs;
}/* end bm_make_gs */

/*
 * ����:����
 * ��ʼ����:2009-7-5
 * �������:2009-7-5
 * ����:
 *    tg:Ҫ�����ڴ�.
 *    tglen:�ڴ�ĳ���.
 *    ptn:ģʽ.
 *    ptnlen:ģʽ����.
 * ����ֵ:
 *	  �ҵ�:����.
 *    δ�ҵ�:-1.
 * ˵��:
 *    BM�㷨��ģʽƥ��.
 */
dword_t bm_matcher(const byte_t *tg, dword_t tglen, const byte_t *ptn, dword_t ptnlen) {
	sdword_t i = 0, j = 0;
	dword_t *bc = NULL, *gs = NULL;
	assert(tg && tglen && ptn && ptnlen);
	/* Ԥ���� */
	bc = bm_make_bc(ptn, ptnlen);
	if (!bc) return -1;
	gs = bm_make_gs(ptn, ptnlen);
	if (!gs) return -1;
	/* Ѱ�� */
	while (j <= tglen - ptnlen) {
		for (i = ptnlen - 1; i >= 0 && ptn[i] == tg[i + j]; i--);
		if (i < 0) {
			logic_free(bc);
			logic_free(gs);
			return j;
			/* j += gs[0]; */
		} else
			j += _max_(gs[i], bc[tg[i + j]] - ptnlen + 1 + i);
	}
	return -1;
}/* end bm_matcher */

/*
 * ����:����
 * ��ʼ����:2009-7-12
 * �������:2009-7-12
 * ����:
 *    tg:Ҫ�����ڴ�.
 *    tglen:�ڴ�ĳ���.
 *    ptn:ģʽ.
 *    ptnlen:ģʽ����.
 *    bc:���ַ���.
 *    gs:�ú�׺��.
 * ����ֵ:
 *	  �ҵ�:����.
 *    δ�ҵ�:-1.
 * ˵��:
 *    BM�㷨��ģʽƥ��.
 */
dword_t bm_matcherex(const byte_t *tg, dword_t tglen, const byte_t *ptn, dword_t ptnlen, dword_t *bc, dword_t *gs) {
	sdword_t i = 0, j = 0;
	assert(tg && tglen && ptn && ptnlen && bc && gs);
	/* Ѱ�� */
	while (j <= tglen - ptnlen) {
		for (i = ptnlen - 1; i >= 0 && ptn[i] == tg[i + j]; i--);
		if (i < 0) {
			return j;
			/* j += gs[0]; */
		} else
			j += _max_(gs[i], bc[tg[i + j]] - ptnlen + 1 + i);
	}
	return -1;
}/* end bm_matcherex */

/*
 * ����:����
 * ��ʼ����:2009-7-12
 * �������:2009-7-12
 * ����:
 * ����ֵ:
 *	  �ɹ�:WM�ṹָ��.
 *    δ�ҵ�:NULL.
 * ˵��:
 *    WM��ʼ��.
 */
PWM init_wm(void) {
	PWM p = (PWM)logic_malloc(sizeof(WM));
	if (!p) return NULL;
	memset(p, 0, sizeof(WM));
	p->smallest = 0xFFFFFFFF;
	return p;
}/* end init_wm */

/*
 * ����:����
 * ��ʼ����:2009-7-12
 * �������:2009-7-12
 * ����:
 *    pwm:ָ��wm��ָ���ָ��.
 * ����ֵ:
 * ˵��:
 *    WM�ͷ�.
 */
void free_wm(PWM *pwm) {
	PWM p = *pwm;
	PWM_PATTERN p_ptn, p_tmp;
	assert(pwm && p);
	if (p->lengths) logic_free(p->lengths);
	if (p->ptn_array) logic_free(p->ptn_array);
	if (p->group_nums) logic_free(p->group_nums);
	if (p->whash) logic_free(p->whash);
	if ((*pwm)->shift_large) logic_free(p->wshift);
	p_ptn = p->ptn_list;
	while (p_ptn) {
		p_tmp = p_ptn->next;
		if (p_ptn->ptn) logic_free(p_ptn->ptn);
		/* �ͷ�BM�ṹ */
		if (p_ptn->bm_ptn) {
			logic_free(p_ptn->bm_ptn->bc);
			logic_free(p_ptn->bm_ptn->gs);
			logic_free(p_ptn->bm_ptn);
		}
		logic_free(p_ptn);
		p_ptn = p_tmp;
	}
	logic_free(p);
	*pwm = NULL;
}/* end pwm */

/*
 * ����:����
 * ��ʼ����:2009-7-12
 * �������:2009-7-12
 * ����:
 *    wm:ָ��wm��ָ��.
 *    id:ģʽ��id.
 *    ptn:ģʽָ��.
 *    ptnlen:ģʽ����.
 * ����ֵ:
 *    �ɹ�:ģʽ�ṹָ��.
 *    ʧ��:NULL.
 * ˵��:
 *    WM����ģʽ.
 */
PWM_PATTERN add_wm_pattern(PWM wm, dword_t id, const byte_t *ptn, dword_t ptnlen) {
	PWM_PATTERN plist = NULL, p = NULL;
	assert(wm && ptn && ptnlen);
	/* Ѱ���Ƿ�ģʽ�Ѿ�����,����Ѽ����򷵻�ģʽ��ָ�� */
	for (plist = wm->ptn_list; plist; plist = plist->next)
		if (plist->ptnlen == ptnlen)
			if (memcmp(ptn, plist->ptn, ptnlen) == 0) return plist;
	/* ����ռ� */
	p = (PWM_PATTERN)logic_malloc(sizeof(WM_PATTERN));
	if (!p) return NULL;
	memset(p, 0, sizeof(WM_PATTERN));
	p->ptn = (byte_t *)logic_malloc(ptnlen);
	if (!(p->ptn)) {
		logic_free(p);
		return NULL;
	}
	p->id = id;
	memcpy(p->ptn, ptn, ptnlen);
	p->ptnlen = ptnlen;
	p->next = wm->ptn_list;
	wm->ptn_list = p;
	
	(wm->max_ptn)++;
	if (ptnlen < wm->smallest) wm->smallest = ptnlen;
	if (ptnlen > wm->largest) wm->largest = ptnlen;
	wm->total += ptnlen;
	wm->avg = wm->total / wm->max_ptn;
	return p;
}/* end add_wm_pattern */

/*
 * ����:����
 * ��ʼ����:2009-7-12
 * �������:2009-7-12
 * ����:
 *    s1:ָ��Ҫ�Աȵ�ָ��.
 *    l1:ָ�볤��.
 *    s2:ָ��Ҫ�Աȵ�ָ��.
 *    l2:ָ�볤��.
 * ����ֵ:
 *    -1:s1 < s2.
 *    0:s1 = s2.
 *    1:s1 > s2.
 * ˵��:
 *    �Ա������ڴ泤��.
 */
static dword_t bcompare(const byte_t *s1, dword_t l1, const byte_t *s2, dword_t l2) {
	dword_t stat; 
	if (l1 == l2) return memcmp(s1, s2, l1);
	else if(l1 < l2) {
		if ((stat = memcmp(s1, s2, l1))) return stat;
		return -1;
	} else {
		if ((stat = memcmp(s1, s2, l2))) return stat;
		return +1;
	}
	return stat;
}/* end bcompare */

/*
 * ����:����
 * ��ʼ����:2009-7-12
 * �������:2009-7-12
 * ����:
 *    s1:ָ��Ҫ�Աȵ�ָ��.
 *    l1:ָ�볤��.
 *    s2:ָ��Ҫ�Աȵ�ָ��.
 *    l2:ָ�볤��.
 * ����ֵ:
 *    -1:s1 < s2.
 *    0:s1 = s2.
 *    1:s1 > s2.
 * ˵��:
 *    �ԱȺ���,����qsort�㷨.
 */
static dword_t qsortcmp(const byte_t *s1, const byte_t *s2) {
	PWM_PATTERN r1= (PWM_PATTERN)s1; 
	PWM_PATTERN r2= (PWM_PATTERN)s2; 
	return bcompare(r1->ptn, r1->ptnlen, r2->ptn, r2->ptnlen);
}/* end qsortcmp */

/*
 * ����:����
 * ��ʼ����:2009-7-12
 * �������:2009-7-12
 * ����:
 *    wm:WM�ṹָ��.
 * ����ֵ:
 *    �ɹ�:T.
 *    ʧ��:F.
 * ˵��:
 *    WMģʽ����.
 */
static bool_t analyze_wm_pattern(PWM wm) {
	dword_t i = 0; 
	wm->lengths = (dword_t *)logic_malloc(sizeof(dword_t) * (wm->largest + 1));
	if (!wm->lengths) return F;
	memset(wm->lengths, 0, sizeof(dword_t) * (wm->largest + 1));
	/* ��¼ÿ��ģʽ���ȵĸ��� */
	for(i = 0; i < wm->max_ptn; i++) wm->lengths[wm->ptn_array[i].ptnlen]++;
	return T;
}/* end analyze_wm_pattern */

/*
 * ����:����
 * ��ʼ����:2009-7-12
 * �������:2009-7-12
 * ����:
 *    wm:WM�ṹָ��.
 * ����ֵ:
 *    �ɹ�:T.
 *    ʧ��:F.
 * ˵��:
 *    WM�㷨Ԥ����,����ģʽ�����ϣ��.
 */
static bool_t build_hashtbl_patgrps(PWM wm) {
	word_t sindex, hindex, ningroup;
	dword_t i;
	/* ����4�ֽڵĹ�ϣ�� */
	wm->hash_entries = HASHTABLESIZE;
	wm->whash = (HASH_TYPE *)logic_malloc(sizeof(HASH_TYPE) * (wm->hash_entries));
	if (!wm->whash) return F;
	/* ����ϣ����ʼֵ */
	for (i = 0; i < wm->hash_entries; i++) wm->whash[i] = (HASH_TYPE) - 1;
	/* ���ֽڹ�ϣ����ʼֵ */
	for (i = 0; i < 0x100; i++) wm->bhash[i] = (HASH_TYPE) - 1;
	/* ����ģʽ����ϣ�� */
	for (i = 0; i < wm->max_ptn; i++) {
		if (wm->ptn_array[i].ptnlen > 1) {
			/* ȡģʽ��ǰ׺ */
			hindex = _hash16_(wm->ptn_array[i].ptn);  
			sindex = wm->whash[hindex] = i;
			ningroup = 1;
			/* �����Դ�ǰ׺��ͷ��ģʽ���� */
			while((++i < wm->max_ptn) && (hindex == _hash16_(wm->ptn_array[i].ptn))) ningroup++;
			wm->group_nums[sindex] = ningroup;
			i--;
		} else if( wm->ptn_array[i].ptnlen == 1) {
			hindex = wm->ptn_array[i].ptn[0];
			sindex = wm->bhash[hindex] = i;
			ningroup = 1;
			/* ���㵥�ֽڵĹ�ϣ�� */
			while((++i < wm->max_ptn) && (hindex == wm->ptn_array[i].ptn[0]) && (wm->ptn_array[i].ptnlen == 1)) ningroup++;
			wm->group_nums[sindex] = ningroup;
			i--;
		}/* end else if */
	}/* end for */
	return T;
}/* end build_hashtbl_patgrps */

/*
 * ����:����
 * ��ʼ����:2009-7-12
 * �������:2009-7-19
 * ����:
 *    wm:WM�ṹָ��.
 * ����ֵ:
 * ˵��:
 *    WM�㷨���ֽڹ����.
 */
static void wm_badchar_tbl(PWM wm) {
	word_t k, m, cindex, shift;
	dword_t small_value = 0xFFFFFFFF, large_value = 0, i;
	/* ������󳤶�����С���� */
	for (i = 0; i < wm->max_ptn; i++) {
		if (wm->ptn_array[i].ptnlen < small_value ) small_value = wm->ptn_array[i].ptnlen; 
		if (wm->ptn_array[i].ptnlen > large_value ) large_value = wm->ptn_array[i].ptnlen; 
	}/* end for */
	m = (word_t)small_value;
	if (m > 255) m = 255;
	wm->shift_len = m;

	/* ��ʼ���ƶ���,�����ƶ�����Ϊ256���ַ� */
	for (i = 0; i < 0x100; i++) wm->bshift[i] = m;
	/* �������ֽ��ƶ� */
	for (i = 0; i < wm->max_ptn; i++) {
		/* ����ÿ��ģʽ */
		for (k = 0; k < m; k++) {
			shift = (m - 1 - k);
			/* ���ֻ����255�ֽڵ��ƶ� */
			if (shift > 255) shift = 255;
			cindex = wm->ptn_array[i].ptn[k];
			/* ��֤��©ƥ��,�ĸ�Сȡ�ĸ� */
			if (shift < wm->bshift[cindex]) wm->bshift[cindex] = shift;
		}/* end for */
	}/* end for */
}/* end wm_badchar_tbl */

/*
 * ����:����
 * ��ʼ����:2009-7-12
 * �������:2009-7-19
 * ����:
 *    wm:WM�ṹָ��.
 * ����ֵ:
 *    �ɹ�:T.
 *    ʧ��:F.
 * ˵��:
 *    WM�㷨���ֹ����.
 */
static bool_t wm_badword_tbl(PWM wm) { 
	word_t shift = 0, k, m, cindex;
	dword_t small_value = 0xFFFFFFFF, large_value = 0, i;

	wm->wshift = (byte_t *)logic_malloc(BWSHIFTABLESIZE * sizeof(byte_t));
	if (!wm->wshift) return F;
	/* ������󳤶�����С���� */
	for (i = 0; i < wm->max_ptn; i++) {
		if (wm->ptn_array[i].ptnlen < small_value ) small_value = wm->ptn_array[i].ptnlen; 
		if (wm->ptn_array[i].ptnlen > large_value ) large_value = wm->ptn_array[i].ptnlen;
	}
	/* ����ƶ�����Ϊ��Сģʽ����,���ǲ��ܳ���255 */
	m = (word_t)small_value;
	if (m > 255) m = 255;  
	wm->shift_len = m;

	/* ��ʼ��Ĭ�ϱ� */ 
	for (i = 0; i < BWSHIFTABLESIZE; i++) wm->wshift[i] = m - 1;

	/* �����ƶ��� */
	for (i = 0; i < wm->max_ptn; i++) {
		for (k = 0; k < m - 1; k++) {
			shift = m - 2 - k;
			/* ���255���ֽڵ��ƶ� */
			if( shift > 255 ) shift = 255;
			/* ����������ƶ��� */
			cindex = (wm->ptn_array[i].ptn[k] | (wm->ptn_array[i].ptn[k+1] << 8));
			if (shift < wm->wshift[cindex]) wm->wshift[cindex] = shift; 
		}/* end for */
	}/* end for */
	return T;
}/* end wm_badword_tbl */

/*
 * ����:����
 * ��ʼ����:2009-7-13
 * �������:2009-7-19
 * ����:
 *    wm:WM�ṹ����.
 *    index:��ϣ���Ĺ�ϣֵ.
 *    text:�������ı�.
 *    currt:��ǰ�ı�.
 *    left:���ı�ͷ��ƫ��.
 *    get1:�������.
 * ����ֵ:
 *    û������:�����ҵ�����Ŀ.
 *    ȫ������:�ҵ�����Ŀ+1ȡ����,+1��ԭ��Ϊʹ���صĽ��Ϊ����,
 *             ���ȫ������,����û���ҵ��κ�ģʽ�򷵻�-(0+1).
 * ˵��:
 *    WMƥ���������.
 */
/* WM������ϣ����ƥ�� */
sdword_t wm_group_matcher(PWM wm, dword_t index, const byte_t *text, const byte_t *currt, dword_t left, wm_get1 get1) {
	dword_t nfound = 0;
	sdword_t k;
	PWM_PATTERN patrn, patrnEnd;
	/* ������ϣ��ģʽ����ȷ���ı���׺ */
	patrn = &wm->ptn_array[index];
	patrnEnd = patrn + wm->group_nums[index];

	/* ѭ��ƥ�������е�ÿ��ģʽ */
	for (; patrn < patrnEnd; patrn++) { 
		byte_t *p, *q;
		/* ���ģʽ�����ı�,ֱ�Ӻ��� */
		if (left < patrn->ptnlen) continue;

		/* �����෴���ַ���ƥ�� */ 
		k = patrn->ptnlen - HASHBYTES16 - 1;
		q = patrn->ptn + HASHBYTES16;      
		p = currt + HASHBYTES16;

		/* �Ա��ַ����Ӻ���ǰ */
		while (k >= 0 && (q[k] == p[k])) k--;
		/* ����ƥ�� */
		if (k < 0) {
			nfound++;
			if (get1) 
				if (get1(patrn->id, (dword_t)(currt - text), patrn->ptn, patrn->ptnlen, text))
					return -(nfound + 1);
		 }/* end if */
	}/* end for */
	return nfound; 
}/* end wm_group_matcher */

/*
 * ����:����
 * ��ʼ����:2009-7-13
 * �������:2009-7-19
 * ����:
 *    wm:WM�ṹָ��.
 *    text:�������ı�.
 *    n:�ı��ĳ���.
 *    get1:�������.
 * ����ֵ:
 *    �ҵ���ģʽ����.
 * ˵��:
 *    WM����������鵥�ֽ�.
 */
dword_t wm_search_normal(PWM wm, const byte_t *text, dword_t n, wm_get1 get1) {
	dword_t left = 0, index = 0, nfound = 0;
	sdword_t ng = 0;
	byte_t *currt, *endt, *cmpt;
	PWM_PATTERN patrn, patrnEnd;

	left = n; 
	endt = text + n;

	/* �ı����ȱ��ƶ��������ȶ� */
	if (n < wm->shift_len) return 0;

	/* ����ÿ���ı��ĺ�׺,�������� */
	for (currt = text, cmpt = text + wm->shift_len - 1; cmpt < endt; currt++, cmpt++, left--) {
		/* ��ⵥ�ֽڵ�ģʽ */
		if ((index = wm->bhash[*currt]) != (HASH_TYPE) - 1) {
			patrn = &wm->ptn_array[index];
			patrnEnd = patrn + wm->group_nums[index];

			for (; patrn < patrnEnd; patrn++) {
				nfound++;
				if (get1) 
					if (get1(wm->ptn_array[index].id, (dword_t)(currt - text), patrn->ptn, patrn->ptnlen, text))
						return nfound;
			}/* end if */
		}/* end if */
		
		/* ���ֻʣһ���ַ���ֱ���˳� */
		if (left == 1) return nfound;

		/* ��ʼ2�ֽ�����ģʽ��Ѱ�� */
		if ((index = wm->whash[((*currt) << 8) | *(currt + 1)]) == (HASH_TYPE) - 1) continue;
		/* ƥ��������ȷ�ĺ�׺ */
		ng = wm_group_matcher(wm, index, text, currt, left, get1);
		if (ng < 0) {
			ng = -ng;
			ng--;
			nfound += ng;
			return nfound; 
		} else
			nfound += ng;
	}/* end for */
	return nfound;
}/* end wm_search_normal */ 

/*
 * ����:����
 * ��ʼ����:2009-7-13
 * �������:2009-7-19
 * ����:
 *    wm:WM�ṹָ��.
 *    text:�������ı�.
 *    n:�ı��ĳ���.
 *    get1:�������.
 * ����ֵ:
 *    �ҵ���ģʽ����.
 * ˵��:
 *    WM��������ʹ�û��ֽڱ�.
 */
dword_t wm_searchex_bc(PWM wm, const byte_t *text, dword_t n, wm_get1 get1) {
	dword_t left, index, nfound, tshift;
	sdword_t ng; 
	byte_t *currt, *endt, *cmpt;
	nfound = 0;

	left = n;
	endt  = text + n;

	/* ����ı�С����С�ƶ��ĳ��� */
	if (n < wm->shift_len) return 0;

	/* ����ÿ���ı��ĺ�׺ */
	for (currt = text, cmpt = text + wm->shift_len - 1; cmpt < endt; currt++, cmpt++, left--) {
		/* ʹ�û��ֽڱ� */
		while ((tshift = wm->bshift[*cmpt]) > 0) {
			cmpt += tshift; currt += tshift; left -= tshift;
			if (cmpt >= endt) return nfound;

			tshift = wm->bshift[*cmpt];
			cmpt += tshift; currt += tshift; left -= tshift;
			if (cmpt >= endt) return nfound;
		}

		/* �������һ���ַ�,1�ֽ��ַ���ƥ���Ѿ����,ֱ�ӷ��� */
		if (left == 1) return nfound;

		/* ����2�ֽ�ǰ׺ */
		if ((index = wm->whash[((*currt)<<8) | *(currt+1)]) == (HASH_TYPE) - 1) continue;

		/* ƥ���׺ */ 
		ng = wm_group_matcher(wm, index, text, currt, left, get1);
		if (ng < 0) {
			ng = -ng;
			ng--;
			nfound += ng; 
			return nfound; 
		} else nfound += ng;
	}/* end for */
	return nfound; 
}/* end wm_searchex_bc */

/*
 * ����:����
 * ��ʼ����:2009-7-13
 * �������:2009-7-19
 * ����:
 *    wm:WM�ṹָ��.
 *    text:�������ı�.
 *    n:�ı��ĳ���.
 *    get1:�������.
 * ����ֵ:
 *    �ҵ���ģʽ����.
 * ˵��:
 *    WM��������ʹ�û��ֱ�.
 */
dword_t wm_searchex_bw(PWM wm, const byte_t *text, dword_t n, wm_get1 get1) {
	dword_t left, index, nfound, tshift;
	sdword_t ng;
	byte_t *currt, *endt, *cmpt;
	nfound = 0;
	left = n;
	endt = text + n; 

	/* ����ı�С����С�ƶ��ĳ��� */
	if (n < wm->shift_len) return 0;

	/* ����ÿ���ı��ĺ�׺ */ 
	for (currt = text, cmpt = text + wm->shift_len - 1; cmpt < endt; currt++, cmpt++, left--) {
		/* ʹ�û��ֱ� */
		tshift = wm->wshift[((*cmpt)<<8) | *(cmpt-1)];
		while (tshift) {
			cmpt += tshift;  currt += tshift; left -= tshift;
			if (cmpt >= endt) return nfound; 
			tshift = wm->wshift[((*cmpt)<<8) | *(cmpt-1)];
		}/* end while */
		if (left == 1) return nfound;  

		if ((index = wm->whash[((*currt)<<8 ) | *(currt+1)]) == (HASH_TYPE) - 1) continue;

		/* ƥ���׺ */
		ng = wm_group_matcher(wm, index, text, currt, left, get1);
		if (ng < 0) { 
			ng = -ng; 
			ng--;
			nfound += ng;
			return nfound; 
		} else nfound += ng;
	}/* end for */
	return nfound;
}/* end wm_searchex_bw */

/*
 * ����:����
 * ��ʼ����:2009-7-12
 * �������:2009-7-12
 * ����:
 *    wm:WM�ṹָ��.
 *    text:�������ı�.
 *    n:�ı��ĳ���.
 *    get1:�������.
 * ����ֵ:
 *    �ɹ�:T.
 *    ʧ��:F.
 * ˵��:
 *    WMԤ����.
 */
bool_t perp_wm_patterns(PWM wm) {
	dword_t i;
	PWM_PATTERN plist;
	assert(wm);

	/* ����ָ��ģʽ����ָ������� */
	wm->ptn_array = (PWM_PATTERN)logic_malloc(sizeof(WM_PATTERN) * (wm->max_ptn));
	if (!wm->ptn_array) return F;
	/* ���ɼ�¼��ϣ����������Ķ��� */
	wm->group_nums = (dword_t *)logic_malloc(sizeof(dword_t) * (wm->max_ptn));
	if (!wm->group_nums) {
		logic_free(wm->ptn_array);
		return F;
	}
	/* �����б��ڵ㵽���� */
	for (i = 0, plist = wm->ptn_list; plist && i < wm->max_ptn; plist = plist->next)
		memcpy(&(wm->ptn_array[i++]), plist, sizeof(WM_PATTERN));

	/* ����ģʽ */
	if (!analyze_wm_pattern(wm)) {
		logic_free(wm->ptn_array);
		logic_free(wm->group_nums);
		return F;
	}

	/* ��ģʽ���� */ 
	qsort(wm->ptn_array, wm->max_ptn, sizeof(WM_PATTERN), qsortcmp); 

	/* ������ϣ��,ģʽ��,Ԥ����Wu - Manber�㷨 */
	if (!build_hashtbl_patgrps(wm)) {
		logic_free(wm->ptn_array);
		logic_free(wm->group_nums);
		logic_free(wm->lengths);
		return F;
	}/* end if */
	
	/* ѡ��ƥ���㷨 */
	if (wm->max_ptn < 5) wm->method = LPM_BM;
	else wm->method = LPM_WM;

	/* ����Wu-Manber�㷨 */
	if (wm->method == LPM_WM) {
		/* �������ֽڷ������ */
		wm_badchar_tbl(wm); 
		/* �������ֹ���� */
		if ((wm->shift_len > 1) && wm->shift_large)
			wm_badword_tbl(wm);

		/* ģʽ�ƶ�Ϊ1���ֽ� */
		if( wm->shift_len == 1) wm->search = wm_search_normal;
		/* ��С��ģʽ > 1�ֽ� */ 
		else if((wm->shift_len > 1) && !wm->shift_large) wm->search = wm_searchex_bc;
		/* ��С��ģʽ > 1�ֽ� - ����ѯ���Ƿ�ʹ�����ƶ����� */ 
		else if((wm->shift_len >  1) && wm->shift_large && wm->wshift) wm->search = wm_searchex_bw;
		/* ��С��ģʽ > 1�ֽ� */
		else wm->search = wm_search_normal;
	}/* end if */

	/* ��ʼ��BM�㷨���� */
	if (wm->method == LPM_BM) {
		dword_t k;
		/* ���䲢����ÿ��ģʽ�Ļ��ַ�����ú�׺�� */
		for (k = 0; k < wm->max_ptn; k++) {
			wm->ptn_array[k].bm_ptn = (PBM_PATTERN)logic_malloc(sizeof(BM_PATTERN));
			if (!wm->ptn_array[k].bm_ptn) {
				logic_free(wm->ptn_array);
				logic_free(wm->group_nums);
				logic_free(wm->lengths);
				return F;
			}
			wm->ptn_array[k].bm_ptn->bc = bm_make_bc(wm->ptn_array[k].ptn, wm->ptn_array[k].ptnlen);
			wm->ptn_array[k].bm_ptn->gs = bm_make_gs(wm->ptn_array[k].ptn, wm->ptn_array[k].ptnlen);
		}/* end for */
	}/* end if */
	return T;
}/* end perp_wm_patterns */

/*
 * ����:����
 * ��ʼ����:2009-7-12
 * �������:2009-7-19
 * ����:
 *    wm:WM�ṹָ��.
 * ����ֵ:
 *    �ɹ�:�����ҵ��ĸ���.
 *    ʧ��:-1.
 * ˵��:
 *    WMƥ��.
 */
dword_t wm_matcher(PWM wm, const byte_t * text, dword_t n, wm_get1 get1) {
	assert(wm && text && n);
	/* ������Ԥ���� */
	if (!perp_wm_patterns(wm)) {
		free_wm(&wm);
		return -1;
	}
	/* �����BM�㷨 */
	if (wm->method == LPM_BM) {
		dword_t i, j, nfound = 0;
		PBM_PATTERN p = NULL;
		for (i = 0; i < wm->max_ptn; i++) {
			p = wm->ptn_array[i].bm_ptn;
			j = bm_matcherex(text, n, wm->ptn_array[i].ptn, wm->ptn_array[i].ptnlen, p->bc, p->gs);
			/* �ҵ� */
			if (j != -1) {
				nfound++;
				if (get1) get1(wm->ptn_array[i].id, j, wm->ptn_array[i].ptn, wm->ptn_array[i].ptnlen, text);
			}/* end if */
		}
		return nfound;
	} else return wm->search(wm, text, n, get1);
}/* end wm_matcher */