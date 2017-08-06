/* -*- C -*- */
/* This file is a part of the omobus-mobile project.
 * Copyright (c) 2006 - 2013 ak-obs, Ltd. <info@omobus.net>.
 * Author: Igor Artemov <i_artemov@ak-obs.ru>.
 *
 * This program is a free software. Redistribution and use in source and binary
 * forms, with or without modification, are permitted provided under the GPLv2 license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include <omobus-mobile.h>

#ifdef HAVE_ZLIB_H
#	include <zlib.h>
#endif //HAVE_ZLIB_H
#ifdef HAVE_BZIP2_H
# include <bzlib.h>
#endif //HAVE_BZIP2_H

/* Дескриптор */
typedef struct {
	short mode;
	void *file;
} pk_handle_t;


/* Открытие пакета. Параметр mode может содержать не только режимы открытия
	 "wb" или "rb", но и уровень сжатия от 0 до 9, например, "wb9". Также 
	 анализируется расширения файла и в зависимости от этого выбирается
	 метод сжатия данных (bz2 - bzip2, gz - deflate, в остальный случаях
	 данные читаются как есть). */
pk_file_t pkopen(const char *path, const char *mode)
{
	pk_handle_t *pk = (pk_handle_t *)malloc(sizeof(pk_handle_t));
	if( pk == NULL )
		return NULL;
	memset(pk, 0, sizeof(pk_handle_t));
	const char *ext = strrchr(path, '.');
	if( ext != NULL ) {
		if( strcmp(ext, ".gz") == 0 ) {
			pk->mode = 1; 
#ifdef HAVE_ZLIB_H			
			pk->file = gzopen(path, mode);
#endif //HAVE_ZLIB_H			
		} else if( strcmp(ext, ".bz2") == 0 ) {
			pk->mode = 2; 
#ifdef HAVE_BZIP2_H
			pk->file = BZ2_bzopen(path, mode);
#endif //HAVE_BZIP2_H			
		}
	}
	if( pk->mode == 0 )
		pk->file = fopen(path, mode);
	if( pk->file == NULL ) {
		free(pk);
		pk = NULL;
	}
	
	return pk;
}

/* Чтение требуемое количество несжатых данных. pkread возвращает количество
	 прочитанных байт, 0 в случае если достигнут конец файла или -1 в случае 
	 ошибки. */
int pkread(pk_file_t file, void *buf, int len)
{
	if( file == NULL )
		return -1;
	pk_handle_t *pk = (pk_handle_t *)file;
	int rc = 0;
	switch( pk->mode ) {
	case 1:
#ifdef HAVE_ZLIB_H		
		rc = gzread(pk->file, buf, len);
#endif //HAVE_ZLIB_H		
		break;
	case 2:
#ifdef HAVE_BZIP2_H
		rc = BZ2_bzread(pk->file, buf, len);
#endif //HAVE_BZIP2_H			
		break;
	default:
		rc = fread(buf, 1, len, pk->file);
	}
	return rc;
}

/* Запись требуемого количества несжатых байт. pkwrite возвращает количество
	 записанных несжатых байт. */
int pkwrite(pk_file_t file, const void *buf, int len)
{
	if( file == NULL )
		return -1;
	pk_handle_t *pk = (pk_handle_t *)file;
	int rc = 0;
	switch( pk->mode ) {
	case 1:
#ifdef HAVE_ZLIB_H
		rc = gzwrite(pk->file, buf, len);
#endif //HAVE_ZLIB_H		
		break;
	case 2:
#ifdef HAVE_BZIP2_H
		rc = BZ2_bzwrite(pk->file, (void *)buf, len);
#endif //HAVE_BZIP2_H			
		break;
	default:
		rc = fwrite(buf, 1, len, pk->file);
	}
	return rc;
}

/* Сброс данных на диск. */
int pkflush(pk_file_t file)
{
	if( file == NULL )
		return -1;
	pk_handle_t *pk = (pk_handle_t *)file;
	int rc = 0;
	switch( pk->mode ) {
	case 1:
#ifdef HAVE_ZLIB_H		
		rc = gzflush(pk->file, Z_SYNC_FLUSH);
#endif //HAVE_ZLIB_H		
		break;
	case 2:
#ifdef HAVE_BZIP2_H
		rc = BZ2_bzflush(pk->file);
#endif //HAVE_BZIP2_H			
		break;
	default:
		rc = fflush(pk->file);
	}
	return rc;
}

/* Закрытие файла. */
int pkclose(pk_file_t file)
{
	if( file == NULL )
		return -1;
	pk_handle_t *pk = (pk_handle_t *)file;
	int rc = 0;
	switch( pk->mode ) {
	case 1:
#ifdef HAVE_ZLIB_H		
		rc = gzclose(pk->file);
#endif //HAVE_ZLIB_H		
		break;
	case 2:
#ifdef HAVE_BZIP2_H
		/*rc = */BZ2_bzclose(pk->file);
#endif //HAVE_BZIP2_H			
		break;
	default:
		rc = fclose(pk->file);
	}
	free(pk);
	return rc;
}
