/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/* Copyright (C) 2018 Hans Petter Jansson
 *
 * This file is part of Chafa, a program that turns images into character art.
 *
 * Chafa is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Chafa is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Chafa.  If not, see <http://www.gnu.org/licenses/>. */

#include "config.h"

#include <string.h>
#include <stdlib.h>
#include <glib.h>
#include "chafa/chafa.h"
#include "chafa/chafa-private.h"

/* Private */

static gint
compare_symbols (const void *a, const void *b)
{
    const ChafaSymbol *a_sym = a;
    const ChafaSymbol *b_sym = b;

    if (a_sym->c < b_sym->c)
        return -1;
    if (a_sym->c > b_sym->c)
        return 1;
    return 0;
}

static void
rebuild_symbols (ChafaSymbolMap *symbol_map)
{
    
    GHashTableIter iter;
    gpointer key, value;
    gint i;

    g_free (symbol_map->symbols);

    symbol_map->n_symbols = g_hash_table_size (symbol_map->desired_symbols);
    symbol_map->symbols = g_new (ChafaSymbol, symbol_map->n_symbols + 1);

    g_hash_table_iter_init (&iter, symbol_map->desired_symbols);
    i = 0;

    while (g_hash_table_iter_next (&iter, &key, &value))
    {
        gint src_index = GPOINTER_TO_INT (key);
        symbol_map->symbols [i++] = chafa_symbols [src_index];
    }

    qsort (symbol_map->symbols, symbol_map->n_symbols, sizeof (ChafaSymbol),
           compare_symbols);
    memset (&symbol_map->symbols [symbol_map->n_symbols], 0, sizeof (ChafaSymbol));

    symbol_map->need_rebuild = FALSE;
}

static GHashTable *
copy_hash_table (GHashTable *src)
{
    GHashTable *dest;
    GHashTableIter iter;
    gpointer key, value;

    dest = g_hash_table_new (g_direct_hash, g_direct_equal);

    g_hash_table_iter_init (&iter, src);
    while (g_hash_table_iter_next (&iter, &key, &value))
    {
        g_hash_table_insert (dest, key, value);
    }

    return dest;
}

void
chafa_symbol_map_init (ChafaSymbolMap *symbol_map)
{
    g_return_if_fail (symbol_map != NULL);

    /* We need the global symbol table */
    chafa_init ();

    memset (symbol_map, 0, sizeof (*symbol_map));
    symbol_map->refs = 1;
}

void
chafa_symbol_map_deinit (ChafaSymbolMap *symbol_map)
{
    g_return_if_fail (symbol_map != NULL);

    if (symbol_map->desired_symbols)
        g_hash_table_unref (symbol_map->desired_symbols);

    g_free (symbol_map->symbols);
}

void
chafa_symbol_map_copy_contents (ChafaSymbolMap *dest, const ChafaSymbolMap *src)
{
    g_return_if_fail (dest != NULL);
    g_return_if_fail (src != NULL);

    memcpy (dest, src, sizeof (*dest));

    if (dest->desired_symbols)
        dest->desired_symbols = copy_hash_table (dest->desired_symbols);

    dest->symbols = NULL;
    dest->need_rebuild = TRUE;
}

void
chafa_symbol_map_prepare (ChafaSymbolMap *symbol_map)
{
    if (!symbol_map->need_rebuild)
        return;

    rebuild_symbols (symbol_map);
}

/* FIXME: Use gunichars as keys in hash table instead, or use binary search here */
gboolean
chafa_symbol_map_has_symbol (const ChafaSymbolMap *symbol_map, gunichar symbol)
{
    gint i;

    g_return_val_if_fail (symbol_map != NULL, FALSE);

    for (i = 0; i < symbol_map->n_symbols; i++)
    {
        const ChafaSymbol *sym = &symbol_map->symbols [i];

        if (sym->c == symbol)
            return TRUE;
        if (sym->c > symbol)
            break;
    }

    return FALSE;
}

/* Public */

ChafaSymbolMap *
chafa_symbol_map_new (void)
{
    ChafaSymbolMap *symbol_map;

    symbol_map = g_new (ChafaSymbolMap, 1);
    chafa_symbol_map_init (symbol_map);
    return symbol_map;
}

void
chafa_symbol_map_ref (ChafaSymbolMap *symbol_map)
{
    g_return_if_fail (symbol_map != NULL);
    g_return_if_fail (symbol_map->refs > 0);

    symbol_map->refs++;
}

void
chafa_symbol_map_unref (ChafaSymbolMap *symbol_map)
{
    g_return_if_fail (symbol_map != NULL);
    g_return_if_fail (symbol_map->refs > 0);

    if (--symbol_map->refs == 0)
    {
        chafa_symbol_map_deinit (symbol_map);
        g_free (symbol_map);
    }
}

void
chafa_symbol_map_add_by_tags (ChafaSymbolMap *symbol_map, ChafaSymbolTags tags)
{
    gint i;

    g_return_if_fail (symbol_map != NULL);
    g_return_if_fail (symbol_map->refs > 0);

    if (!symbol_map->desired_symbols)
        symbol_map->desired_symbols = g_hash_table_new (g_direct_hash, g_direct_equal);

    for (i = 0; chafa_symbols [i].c != 0; i++)
    {
        if (chafa_symbols [i].sc & tags)
            g_hash_table_add (symbol_map->desired_symbols, GINT_TO_POINTER (i));
    }

    symbol_map->need_rebuild = TRUE;
}

void
chafa_symbol_map_remove_by_tags (ChafaSymbolMap *symbol_map, ChafaSymbolTags tags)
{
    gint i;

    g_return_if_fail (symbol_map != NULL);
    g_return_if_fail (symbol_map->refs > 0);

    if (!symbol_map->desired_symbols)
        symbol_map->desired_symbols = g_hash_table_new (g_direct_hash, g_direct_equal);

    for (i = 0; chafa_symbols [i].c != 0; i++)
    {
        if (chafa_symbols [i].sc & tags)
            g_hash_table_remove (symbol_map->desired_symbols, GINT_TO_POINTER (i));
    }

    symbol_map->need_rebuild = TRUE;
}