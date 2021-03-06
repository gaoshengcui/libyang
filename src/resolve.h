/**
 * @file resolve.h
 * @author Michal Vasko <mvasko@cesnet.cz>
 * @brief libyang resolve header
 *
 * Copyright (c) 2015 CESNET, z.s.p.o.
 *
 * This source code is licensed under BSD 3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://opensource.org/licenses/BSD-3-Clause
 */

#ifndef _RESOLVE_H
#define _RESOLVE_H

#include "libyang.h"
#include "extensions.h"

/**
 * @brief Type of an unresolved item (in either SCHEMA or DATA)
 */
enum UNRES_ITEM {
    /* SCHEMA */
    UNRES_USES,          /* unresolved uses grouping (refines and augments in it are resolved as well) */
    UNRES_IFFEAT,        /* unresolved if-feature */
    UNRES_TYPE_DER,      /* unresolved derived type defined in leaf/leaflist */
    UNRES_TYPE_DER_TPDF, /* unresolved derived type defined as typedef */
    UNRES_TYPE_DER_EXT,
    UNRES_TYPE_LEAFREF,  /* check leafref value */
    UNRES_AUGMENT,       /* unresolved augment targets */
    UNRES_CHOICE_DFLT,   /* check choice default case */
    UNRES_IDENT,         /* unresolved derived identities */
    UNRES_TYPE_IDENTREF, /* check identityref value */
    UNRES_FEATURE,       /* feature for circular check, it must be postponed when all if-features are resolved */
    UNRES_TYPEDEF_DFLT,  /* validate default type value (from typedef) */
    UNRES_TYPE_DFLT,     /* validate default type value (from lys_node) */
    UNRES_LIST_KEYS,     /* list keys */
    UNRES_LIST_UNIQ,     /* list uniques */
    UNRES_XPATH,         /* unchecked XPath expression */
    UNRES_EXT,           /* extension instances */

    UNRES_EXT_FINALIZE,  /* extension is already resolved, but needs to be finalized via plugin callbacks */

    /* DATA */
    UNRES_LEAFREF,       /* unresolved leafref reference */
    UNRES_INSTID,        /* unresolved instance-identifier reference */
    UNRES_WHEN,          /* unresolved when condition */
    UNRES_MUST,          /* unresolved must condition */
    UNRES_MUST_INOUT,    /* unresolved must condition in parent input or output */
    UNRES_UNION,         /* union with leafref which must be checked because the type can change without changing the
                            value itself, but removing the target node */

    /* generic */
    UNRES_RESOLVED,      /* a resolved item */
    UNRES_DELETE,        /* prepared for auto-delete */
};

/**
 * @brief auxiliaty structure to hold all necessary information for UNRES_EXT
 */
struct unres_ext {
    union {
        struct lyxml_elem *yin;         /**< YIN content of the extension instance */
        struct yang_ext_substmt *yang;  /**< YANG content of strings */
    } data;
    LYS_INFORMAT datatype;              /**< type of the data in data union */

    /* data for lys_ext_instance structure */
    void *parent;
    struct lys_module *mod;
    LYEXT_PAR parent_type;
    LYEXT_SUBSTMT substmt;
    uint8_t substmt_index;
    uint8_t ext_index;
};

/**
 * @brief auxiliary structure to hold all necessary information for UNRES_LIST_UNIQ
 */
struct unres_list_uniq {
    struct lys_node *list;
    const char *expr;
    uint8_t *trg_type;
};

/**
 * @brief Unresolved items in DATA
 */
struct unres_data {
    struct lyd_node **node;
    enum UNRES_ITEM *type;
    uint32_t count;
};

/**
 * @brief Unresolved items in a SCHEMA
 */
struct unres_schema {
    void **item;            /* array of pointers, each is determined by the type (one of lys_* structures) */
    enum UNRES_ITEM *type;  /* array of unres types */
    void **str_snode;       /* array of pointers, each is determined by the type (a string, a lys_node *, or NULL) */
    struct lys_module **module; /* array of pointers to the item's module */
    uint32_t count;         /* count of unres items */
};

struct len_ran_intv {
    /* 0 - unsigned, 1 - signed, 2 - floating point */
    uint8_t kind;
    union {
        struct {
            uint64_t min;
            uint64_t max;
        } uval;

        struct {
            int64_t min;
            int64_t max;
        } sval;

        struct {
            int64_t min;
            int64_t max;
        } fval;
    } value;

    struct lys_type *type;     /* just to be able to get to optional error-message and/or error-app-tag */
    struct len_ran_intv *next;
};

/**
 * @brief Convert a string with a decimal64 value into our representation.
 * Syntax is expected to be correct. Does not log.
 *
 * @param[in,out] str_num Pointer to the beginning of the decimal64 number, returns the first unparsed character.
 * @param[in] dig Fraction-digits of the resulting number.
 * @param[out] num Decimal64 base value, fraction-digits equal \p dig.
 * @return 0 on success, non-zero on error.
 */
int parse_range_dec64(const char **str_num, uint8_t dig, int64_t *num);

int parse_identifier(const char *id);

int parse_schema_nodeid(const char *id, const char **mod_name, int *mod_name_len, const char **name, int *nam_len,
                        int *is_relative, int *has_predicate, int *all_desc, int extended);

int parse_schema_json_predicate(const char *id, const char **mod_name, int *mod_name_len, const char **name,
                                int *nam_len, const char **value, int *val_len, int *has_predicate);

/**
 * @param[in] expr compiled if-feature expression
 * @return 1 if enabled, 0 if disabled
 */
int resolve_iffeature(struct lys_iffeature *expr);
void resolve_iffeature_getsizes(struct lys_iffeature *iffeat, unsigned int *expr_size, unsigned int *feat_size);
int resolve_iffeature_compile(struct lys_iffeature *iffeat_expr, const char *value, struct lys_node *node,
                              int infeature, struct unres_schema *unres);
uint8_t iff_getop(uint8_t *list, int pos);

int inherit_config_flag(struct lys_node *node, int flags, int clear);

void resolve_identity_backlink_update(struct lys_ident *der, struct lys_ident *base);

struct lyd_node *resolve_data_descendant_schema_nodeid(const char *nodeid, struct lyd_node *start);

int resolve_schema_nodeid(const char *nodeid, const struct lys_node *start, const struct lys_module *cur_module,
                          struct ly_set **ret, int extended, int no_node_error);

int resolve_descendant_schema_nodeid(const char *nodeid, const struct lys_node *start, int ret_nodetype,
                                     int no_innerlist, const struct lys_node **ret);

int resolve_choice_default_schema_nodeid(const char *nodeid, const struct lys_node *start, const struct lys_node **ret);

int resolve_absolute_schema_nodeid(const char *nodeid, const struct lys_module *module, int ret_nodetype,
                                   const struct lys_node **ret);

const struct lys_node *resolve_json_nodeid(const char *nodeid, struct ly_ctx *ctx, const struct lys_node *start, int output);

struct lyd_node *resolve_partial_json_data_nodeid(const char *nodeid, const char *llist_value, struct lyd_node *start,
                                                  int options, int *parsed);

int resolve_len_ran_interval(const char *str_restr, struct lys_type *type, struct len_ran_intv **ret);

int resolve_superior_type(const char *name, const char *prefix, const struct lys_module *module,
                          const struct lys_node *parent, struct lys_tpdf **ret);

int resolve_unique(struct lys_node *parent, const char *uniq_str_path, uint8_t *trg_type);

void resolve_when_ctx_snode(const struct lys_node *schema, struct lys_node **ctx_snode,
                            enum lyxp_node_type *ctx_snode_type);

/* get know if resolve_when() is applicable to the node (there is when condition connected with this node)
 *
 * @param[in] mode 0 - search for when in parent until there is another possible data node
 *                 2 - search for when until reached the stop node, if NULL, search in all parents
 */
int resolve_applies_when(const struct lys_node *schema, int mode, const struct lys_node *stop);

/* return: 0x0 - no applicable must,
 *         0x1 - node's schema has must,
 *         0x2 - node's parent is inout with must,
 *         0x3 - 0x2 & 0x1 combined */
int resolve_applies_must(const struct lyd_node *node);

struct lys_ident *resolve_identref(struct lys_type *type, const char *ident_name, struct lyd_node *node,
                                   struct lys_module *mod, int dflt);

int resolve_unres_schema(struct lys_module *mod, struct unres_schema *unres);

int resolve_when(struct lyd_node *node, int ignore_fail, struct lys_when **failed_when);

int unres_schema_add_str(struct lys_module *mod, struct unres_schema *unres, void *item, enum UNRES_ITEM type,
                         const char *str);

int unres_schema_add_node(struct lys_module *mod, struct unres_schema *unres, void *item, enum UNRES_ITEM type,
                          struct lys_node *node);

int unres_schema_dup(struct lys_module *mod, struct unres_schema *unres, void *item, enum UNRES_ITEM type,
                     void *new_item);

/* start_on_backwards - unres is searched from the end to beginning, so the search will start
 *                      on start_on_backwards index in unres (unless -1) and skip indices
 *                      larger than start_on_backards
 */
int unres_schema_find(struct unres_schema *unres, int start_on_backwards, void *item, enum UNRES_ITEM type);

void unres_schema_free(struct lys_module *module, struct unres_schema **unres, int all);

int resolve_union(struct lyd_node_leaf_list *leaf, struct lys_type *type, int store, int ignore_fail,
                  struct lys_type **resolved_type);

int resolve_unres_data_item(struct lyd_node *dnode, enum UNRES_ITEM type, int ignore_fail, struct lys_when **failed_when);

int unres_data_addonly(struct unres_data *unres, struct lyd_node *node, enum UNRES_ITEM type);
int unres_data_add(struct unres_data *unres, struct lyd_node *node, enum UNRES_ITEM type);
void unres_data_del(struct unres_data *unres, uint32_t i);

int resolve_unres_data(struct unres_data *unres, struct lyd_node **root, int options);
int schema_nodeid_siblingcheck(const struct lys_node *sibling, const struct lys_module *cur_module,
                           const char *mod_name, int mod_name_len, const char *name, int nam_len);

#endif /* _RESOLVE_H */
