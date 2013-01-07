#include <config.h>
#include "hashtable.h"
#include "util.h"
#include <assert.h>
#include <memory.h>


#define FIRST_CELL(hash) (m_cells + ((hash) & (m_arraySize - 1)))
#define CIRCULAR_NEXT(c) ((c) + 1 != m_cells + m_arraySize ? (c) + 1 : m_cells)
#define CIRCULAR_OFFSET(a, b) ((b) >= (a) ? (b) - (a) : m_arraySize + (b) - (a))


//----------------------------------------------
//  HashTable::HashTable
//----------------------------------------------
HashTable::HashTable(size_t initialSize)
{
    // Initialize regular cells
    m_arraySize = initialSize;
    assert((m_arraySize & (m_arraySize - 1)) == 0);   // Must be a power of 2
    m_cells = new Cell[m_arraySize];
    memset(m_cells, 0, sizeof(Cell) * m_arraySize);
    m_population = 0;

    // Initialize zero cell
    m_zeroUsed = 0;
    m_zeroCell.key = 0;
    m_zeroCell.value = 0;
}

//----------------------------------------------
//  HashTable::~HashTable
//----------------------------------------------
HashTable::~HashTable()
{
    // Delete regular cells
    delete[] m_cells;
}

//----------------------------------------------
//  HashTable::Lookup
//----------------------------------------------
HashTable::Cell* HashTable::Lookup(size_t key)
{
    if (key)
    {
        // Check regular cells
        for (Cell* cell = FIRST_CELL(integerHash(key));; cell = CIRCULAR_NEXT(cell))
        {
            if (cell->key == key)
                return cell;
            if (!cell->key)
                return NULL;
        }
    }
    else
    {
        // Check zero cell
        if (m_zeroUsed)
            return &m_zeroCell;
        return NULL;
    }
};

//----------------------------------------------
//  HashTable::Insert
//----------------------------------------------
HashTable::Cell* HashTable::Insert(size_t key)
{
    if (key)
    {
        // Check regular cells
        for (;;)
        {
            for (Cell* cell = FIRST_CELL(integerHash(key));; cell = CIRCULAR_NEXT(cell))
            {
                if (cell->key == key)
                    return cell;        // Found
                if (cell->key == 0)
                {
                    // Insert here
                    if ((m_population + 1) * 4 >= m_arraySize * 3)
                    {
                        // Time to resize
                        Repopulate(m_arraySize * 2);
                        break;
                    }
                    ++m_population;
                    cell->key = key;
                    return cell;
                }
            }
        }
    }
    else
    {
        // Check zero cell
        if (!m_zeroUsed)
        {
            // Insert here
            m_zeroUsed = true;
            if (++m_population * 4 >= m_arraySize * 3)
			{
				// Even though we didn't use a regular slot, let's keep the sizing rules consistent
                Repopulate(m_arraySize * 2);
			}
        }
        return &m_zeroCell;
    }
}

//----------------------------------------------
//  HashTable::Delete
//----------------------------------------------
void HashTable::Delete(Cell* cell)
{
    if (cell != &m_zeroCell)
    {
        // Delete from regular cells
        assert(cell >= m_cells && cell - m_cells < m_arraySize);
        assert(cell->key);

        // Remove this cell by shuffling neighboring cells so there are no gaps in anyone's probe chain
        for (Cell* neighbor = CIRCULAR_NEXT(cell);; neighbor = CIRCULAR_NEXT(neighbor))
        {
            if (!neighbor->key)
            {
                // There's nobody to swap with. Go ahead and clear this cell, then return
                cell->key = 0;
                cell->value = 0;
                m_population--;
                return;
            }
            Cell* ideal = FIRST_CELL(integerHash(neighbor->key));
            if (CIRCULAR_OFFSET(ideal, cell) < CIRCULAR_OFFSET(ideal, neighbor))
            {
                // Swap with neighbor, then make neighbor the new cell to remove.
                *cell = *neighbor;
                cell = neighbor;
            }
        }
    }
    else
    {
        // Delete zero cell
        assert(m_zeroUsed);
        m_zeroUsed = false;
        cell->value = 0;
        m_population--;
        return;
    }
}

//----------------------------------------------
//  HashTable::Clear
//----------------------------------------------
void HashTable::Clear()
{
    // (Does not resize the array)
    // Clear regular cells
    memset(m_cells, 0, sizeof(Cell) * m_arraySize);
    m_population = 0;
    // Clear zero cell
    m_zeroUsed = false;
    m_zeroCell.value = 0;
}

//----------------------------------------------
//  HashTable::Compact
//----------------------------------------------
void HashTable::Compact()
{
    Repopulate(upper_power_of_two((m_population * 4 + 3) / 3));
}

//----------------------------------------------
//  HashTable::Repopulate
//----------------------------------------------
void HashTable::Repopulate(size_t desiredSize)
{
    assert((desiredSize & (desiredSize - 1)) == 0);   // Must be a power of 2
    assert(m_population * 4  <= desiredSize * 3);

    // Get start/end pointers of old array
    Cell* oldCells = m_cells;
    Cell* end = m_cells + m_arraySize;

    // Allocate new array
    m_arraySize = desiredSize;
    m_cells = new Cell[m_arraySize];
    memset(m_cells, 0, sizeof(Cell) * m_arraySize);

    // Iterate through old array
    for (Cell* c = oldCells; c != end; c++)
    {
        if (c->key)
        {
            // Insert this element into new array
            for (Cell* cell = FIRST_CELL(integerHash(c->key));; cell = CIRCULAR_NEXT(cell))
            {
                if (!cell->key)
                {
                    // Insert here
                    *cell = *c;
                    break;
                }
            }
        }
    }

    // Delete old array
    delete[] oldCells;
}

//----------------------------------------------
//  Iterator::Iterator
//----------------------------------------------
HashTable::Iterator::Iterator(HashTable &table) : m_table(table)
{
    m_cur = &m_table.m_zeroCell;
    if (!m_table.m_zeroUsed)
        Next();
}

//----------------------------------------------
//  Iterator::Next
//----------------------------------------------
HashTable::Cell* HashTable::Iterator::Next()
{
    // Already finished?
    if (!m_cur)
        return m_cur;

    // Iterate past zero cell
    if (m_cur == &m_table.m_zeroCell)
        m_cur = &m_table.m_cells[-1];

    // Iterate through the regular cells
    Cell* end = m_table.m_cells + m_table.m_arraySize;
    while (++m_cur != end)
    {
        if (m_cur->key)
            return m_cur;
    }

    // Finished
    return m_cur = NULL;
}
