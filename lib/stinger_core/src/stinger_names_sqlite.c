#include "xmalloc.h"
#include "x86_full_empty.h"
#include "stinger_names.h"
#include "stinger_error.h"

#include <stdint.h>
#include <string.h>

#ifdef NAME_USE_SQLITE

/**
  * Author: Jason Poovey
 */
#define MAP_SN(X) \
    sqlite3 * db = (sqlite3 *)((X)->db); \
    int64_t * next_type_arr = (int64_t *)((X)->storage + (X)->next_type_start);

/**
 * @brief Allocates a stinger_names_t and initialzies it.
 *
 * @param max_types The maximum number of types supported.
 *
 * @return A new stinger_names_t.
 */
stinger_names_t *
stinger_names_new(int64_t max_types) {
  stinger_names_t * sn = xcalloc(sizeof(stinger_names_t) +
    ((max_types+1) * sizeof(int64_t) * 1), sizeof(uint8_t));
  stinger_names_init(sn,max_types);
  return sn;
}

void
stinger_names_init(stinger_names_t * sn, int64_t max_types) {
  // Open the database
  int rc = sqlite3_open("file:namesdb?mode=memory&cache=private", &sn->db);

  if ( rc ) {
    LOG_E_A("Can't open names database: %s\n", sqlite3_errmsg(sn->db));
  }

  const char * drop_sql =
      "DROP TABLE IF EXISTS NAMES";

  sqlite3_exec(sn->db, drop_sql, NULL, NULL, NULL);

  char * create_db = "CREATE TABLE IF NOT EXISTS NAMES(uid INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, NAME TEXT NOT NULL, ID BIGINT NOT NULL)";

  rc = sqlite3_exec(sn->db, create_db, NULL, NULL, NULL);

  char * index_sql = "CREATE INDEX IF NOT EXISTS NameIdx ON NAMES (NAME)";
  char * index_sql1 = "CREATE INDEX IF NOT EXISTS IDIdx on NAMES (ID)";

  rc = sqlite3_exec(sn->db, index_sql, NULL, NULL, NULL);
  rc = sqlite3_exec(sn->db, index_sql1, NULL, NULL, NULL);

  sn->max_types = max_types;
  sn->num_types = 0;
  sn->next_type_idx = 0;
  sn->free_type_idx = max_types;

  int64_t * next_type = (int64_t *)(sn->storage + sn->next_type_start);

  for (int64_t i = 0; i < max_types; i++) {
    next_type[i] = i;
  }
  next_type[max_types] = NAME_EMPTY_TYPE;

  return;
}

/**
 * @brief Resizes a stinger_names to a larger size
 *
 * @param double pointer to the stinger_names to resize
 * @param max_types The maximum number of types supported.
 */
void
stinger_names_resize(stinger_names_t ** sn, int64_t max_types) {
    stinger_names_t * old_sn = *sn;

    if (max_types <= (old_sn)->max_types) {
      return;
    }

    stinger_names_t * new_sn = stinger_names_new(max_types);

    // Sync types array
    new_sn->num_types = (old_sn)->num_types;

    int64_t nti = (old_sn)->next_type_idx;
    int64_t fti = (old_sn)->free_type_idx;

    int64_t i = 0;

    int64_t * old_nt = (int64_t *)((old_sn)->storage + (old_sn)->next_type_start);
    int64_t * new_nt = (int64_t *)((new_sn)->storage + (new_sn)->next_type_start);

    while(nti != fti) {
      new_nt[i] = old_nt[nti];
      nti = (nti + 1) % ((old_sn)->max_types+1);
      i++;
    }

    for (int64_t j=0; j < (max_types-(old_sn)->max_types); j++) {
      new_nt[i] = (old_sn)->max_types + j;
      i++;
    }

    new_sn->next_type_idx = 0;
    new_sn->free_type_idx = i;

    for (i; i < (max_types+1); i++) {
      new_nt[i] = NAME_EMPTY_TYPE;
    }

    // Close old db handle
    stinger_names_free(&old_sn);
    *sn = new_sn;
}

size_t
stinger_names_size(int64_t max_types) {
  size_t rtn = sizeof(stinger_names_t) +
    (max_types * sizeof(int64_t) * 1); /* to_int */
  return rtn;
}

/**
 * @brief Free the stinger_names_t and set the pointer to NULL.
 *
 * @param sn A pointer to a stinger_names struct
 *
 * @return NULL
 */
stinger_names_t *
stinger_names_free(stinger_names_t ** sn) {
  if(sn && *sn) {
    const char * drop_sql =
      "DROP TABLE IF EXISTS NAMES";

    sqlite3_exec((*sn)->db, drop_sql, NULL, NULL, NULL);

    sqlite3_close((*sn)->db);
    free(*sn);
    *sn = NULL;
  }
  return NULL;
}

/**
 * @brief Create new mapping for a name (if one did not previously exist)
 *
 * @param sn A pointer to a stinger_names struct
 * @param name The string name of the type you wish to create or lookup.
 * @param out A pointer to hold the return value of the mapping (The type on creation success or -1 on failure)
 *
 * @return 1 if the creation was successful, 0 if a mapping already exists, -1 if the mapping fails
 */
int
stinger_names_create_type(stinger_names_t * sn, const char * name, int64_t * out) {
  MAP_SN(sn)

  int64_t type = stinger_names_lookup_type(sn,name);
  if (type != -1) {
    *out = type;
    return 0;
  }

  int64_t next_type_idx = readfe(&sn->next_type_idx);
  int64_t next_type = *(next_type_arr + next_type_idx);

  if (next_type == NAME_EMPTY_TYPE) {
    writeef(&sn->next_type_idx,next_type_idx);
    *out = -1;
    return -1;
  } else {
    *(next_type_arr + next_type_idx) = NAME_EMPTY_TYPE;
    next_type_idx = (next_type_idx + 1) % (sn->max_types + 1);
    writeef(&sn->next_type_idx,next_type_idx);

    *out = next_type;

    //fprintf(stderr,"SQLITE insert - %s - %ld\n",name,next_type);

    const char * create_sql =
      "INSERT OR IGNORE INTO NAMES "
          "(NAME, ID)"
      " VALUES "
        "(?1, ?2)";

    sqlite3_stmt * create_stmt;

    int rc;

    rc = sqlite3_prepare_v2(sn->db, create_sql, -1, &create_stmt, NULL);

    rc = sqlite3_bind_text(create_stmt,1,name,-1,SQLITE_STATIC);
    rc = sqlite3_bind_int64(create_stmt,2,next_type);
    rc = sqlite3_step(create_stmt);
    if ( rc != SQLITE_DONE && rc != SQLITE_OK) {
      LOG_E_A("Can't insert data: %d", rc);
    }
    sqlite3_finalize(create_stmt);
    return 1;
  }
}

/**
 * @brief Lookup a type mapping.
 *
 * @param sn A pointer to a stinger_names struct
 * @param name The string name of the type you wish to lookup.
 *
 * @return The type on success or -1 if the type does not exist.
 */
int64_t
stinger_names_lookup_type(stinger_names_t * sn, const char * name) {
  MAP_SN(sn)

  const char * lookup_id_sql =
    "SELECT "
        " ID "
    " FROM "
      " NAMES "
    " WHERE "
      " NAME = ? ";

  sqlite3_stmt * lookup_id_stmt;

  int rc;

  rc = sqlite3_prepare_v2(sn->db, lookup_id_sql, -1, &lookup_id_stmt, NULL);

  rc = sqlite3_bind_text(lookup_id_stmt,1,name,-1,SQLITE_TRANSIENT);
  rc = sqlite3_step(lookup_id_stmt);

  if (rc == SQLITE_ROW) {
    int64_t id = sqlite3_column_int64(lookup_id_stmt,0);
    //fprintf(stderr,"SQLITE - %s - %ld\n",name,id);
    sqlite3_finalize(lookup_id_stmt);
    return id;
  } else {
    sqlite3_finalize(lookup_id_stmt);
    return -1;
  }
}

/**
 * @brief Lookup the corresponding string for a given type
 *
 * @param sn A pointer to a stinger_names struct
 * @param type The integral type you want to look up.
 *
 * @return Returns string if the mapping exists or NULL.
 */
char *
stinger_names_lookup_name(stinger_names_t * sn, int64_t type) {
  MAP_SN(sn)

  const char * lookup_name_sql =
    "SELECT "
        "NAME "
    "FROM "
      "NAMES "
    " WHERE "
      "ID = ?";

  sqlite3_stmt * lookup_name_stmt;

  int rc;

  rc =sqlite3_prepare_v2(sn->db, lookup_name_sql, -1, &lookup_name_stmt, NULL);

  rc = sqlite3_bind_int64(lookup_name_stmt,1,type);
  rc = sqlite3_step(lookup_name_stmt);

  if (rc == SQLITE_ROW) {
    const char * s_name = sqlite3_column_text(lookup_name_stmt,0);
    char * name = (char *)calloc(strlen(s_name) + 1,sizeof(char*));
    strcpy(name,s_name);
    sqlite3_finalize(lookup_name_stmt);
    return name;
  } else {
    sqlite3_finalize(lookup_name_stmt);
    return NULL;
  }
}

/**
 * @brief Delete the corresponding string for a given type
 *
 * @param sn A pointer to a stinger_names struct
 * @param type The integral type you want to delete
 *
 * @return Returns 0 if the statement completed successfully (this will occur even if the type doesn't exist)
 *    Returns -1 if there is a sqlite error.
 */
int64_t
stinger_names_remove_type(stinger_names_t * sn, int64_t type) {
  MAP_SN(sn);

  const char * remove_type_sql = "DELETE FROM NAMES WHERE ID = ?";

  sqlite3_stmt * delete_stmt;

  sqlite3_prepare_v2(sn->db, remove_type_sql, -1, &delete_stmt, NULL);

  sqlite3_bind_int64(delete_stmt,1,type);
  int rc = sqlite3_step(delete_stmt);
  sqlite3_finalize(delete_stmt);

  if (rc == SQLITE_DONE) {
    return 0;
  } else {
    return -1;
  }
}

/**
 * @brief Delete the corresponding string for a given string
 *
 * @param sn A pointer to a stinger_names struct
 * @param name The name you want to delete
 *
 * @return Returns 0 if the statement completed successfully (this will occur even if the type doesn't exist)
 *    Returns -1 if there is a sqlite error.
 */
int64_t
stinger_names_remove_name(stinger_names_t * sn, const char * name) {
  MAP_SN(sn);

  const char * remove_type_sql = "DELETE FROM NAMES WHERE NAME = ?";

  sqlite3_stmt * delete_stmt;

  sqlite3_prepare_v2(sn->db, remove_type_sql, -1, &delete_stmt, NULL);

  sqlite3_bind_text(delete_stmt,1,name,-1,SQLITE_TRANSIENT);
  int rc = sqlite3_step(delete_stmt);
  sqlite3_finalize(delete_stmt);

  if (rc == SQLITE_DONE) {
    return 0;
  } else {
    return -1;
  }
}

int64_t
stinger_names_count(stinger_names_t * sn) {
  sqlite3_stmt * stmt;

  const char * count_sql =
      "SELECT "
          "COUNT(*) "
      "FROM "
        "NAMES";

  sqlite3_prepare_v2(sn->db, count_sql, -1, &stmt, NULL);
  int rc = sqlite3_step(stmt);

  if (rc == SQLITE_ROW) {
    int64_t count = sqlite3_column_int64(stmt,0);
    sqlite3_finalize(stmt);
    return count;
  } else {
    sqlite3_finalize(stmt);
    return 0;
  }
}

void
stinger_names_print(stinger_names_t * sn) {
  MAP_SN(sn)

  sqlite3_stmt * stmt;

  const char * lookup_sql =
    "SELECT "
      "NAME, ID "
    "FROM "
      "NAMES";

  sqlite3_prepare_v2(sn->db, lookup_sql, -1, &stmt, NULL);

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    char * name = sqlite3_column_text(stmt,0);
    int64_t id = sqlite3_column_int64(stmt,1);
    printf("%s %ld\n",name,id);
  }
}


/**
 * @brief Save the stinger_names
 */
void
stinger_names_save(stinger_names_t * sn, FILE * fp) {
  MAP_SN(sn)

  LOG_E("Saving of stinger names is unsupported in sqlite3 mode");
}

/**
 * @brief Load the stinger_names
 */
void
stinger_names_load(stinger_names_t * sn, FILE * fp) {
  LOG_E("Loading of stinger names is unsupported in sqlite3 mode");
  return;
}

#if defined(STINGER_stinger_names_TEST)
#include <stdio.h>

int main(int argc, char *argv[]) {
  stinger_names_t * sn = stinger_names_new(12);

  int64_t t = stinger_names_create_type(sn, "This is a test");
  if(t < 0) {
    printf("Failed to create!\n");
    stinger_names_print(sn);
    return -1;
  }

  if(t != stinger_names_create_type(sn, "This is a test")) {
    printf("Failed when creating duplicate!\n");
    stinger_names_print(sn);
    return -1;
  }

  if(t != stinger_names_lookup_type(sn, "This is a test")) {
    printf("Failed when looking up existing!\n");
    stinger_names_print(sn);
    return -1;
  }

  if(t+1 != stinger_names_create_type(sn, "This is a test 2")) {
    printf("Failed when creating new type!\n");
    stinger_names_print(sn);
    return -1;
  }

  if(-1 != stinger_names_lookup_type(sn, "This wrong")) {
    printf("Failed when looking up non-existant!\n");
    stinger_names_print(sn);
    return -1;
  }

  if(strcmp("This is a test", stinger_names_lookup_name(sn, t))) {
    printf("Failed when looking up name!\n");
    stinger_names_print(sn);
    return -1;
  }

  stinger_names_free(&sn);
  return 0;
}
#endif

#endif
