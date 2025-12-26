# Generated Database Headers

These files are auto-generated from the Prisma schema. Do not edit manually.

## Regeneration

To regenerate these files after schema changes:

```bash
cd /home/strider/Code/mud/fierymud
python scripts/generate_db_headers.py
```

## Source

Schema: `../muditor/packages/db/prisma/schema.prisma`

## Files

- `db_enums.hpp` - All enum definitions with from_db/to_db converters
- `db_tables.hpp` - Table and column name constants

## Usage

```cpp
#include "database/generated/db_enums.hpp"
#include "database/generated/db_tables.hpp"

// Use column constants
auto name = row[db::Room::NAME].as<std::string>();

// Use enum converters
auto flag = db::room_flag_from_db("NO_MOB");
if (flag) {
    room->set_flag(*flag);
}
```

Last generated: 2025-12-25T16:07:25.717654
