# FieryMUD Web Interface - System Design

## Overview

A modern web application for viewing, editing, and managing FieryMUD world content with real-time synchronization and visual representation.

## Architecture

### High-Level System Design
```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   Frontend      │    │   Backend API   │    │   FieryMUD      │
│   React SPA     │◄──►│   Node.js/      │◄──►│   C++ Core      │
│                 │    │   Express       │    │                 │
└─────────────────┘    └─────────────────┘    └─────────────────┘
        │                        │                        │
        │                        │                        │
        ▼                        ▼                        ▼
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   State Mgmt    │    │   Database      │    │   File System   │
│   React Query   │    │   PostgreSQL    │    │   lib/world/    │
│   Zustand       │    │   Redis Cache   │    │   lib/players/  │
└─────────────────┘    └─────────────────┘    └─────────────────┘
```

### Technology Stack
- **Frontend**: React 18, TypeScript, Tailwind CSS, React Query, Zustand
- **Backend**: Node.js 20, Express.js, TypeScript, Prisma ORM
- **Database**: PostgreSQL 15 (primary), Redis 7 (cache)
- **Real-time**: Socket.io, Server-Sent Events
- **Visualization**: D3.js, React Flow, Three.js
- **Authentication**: JWT with role-based access

## Data Architecture

### Core Entities
```typescript
interface Character {
  id: string;
  name: string;
  class: CharacterClass;
  level: number;
  experience: number;
  stats: CharacterStats;
  equipment: Equipment[];
  inventory: Item[];
  location: RoomReference;
  online: boolean;
  lastLogin: Date;
  clan?: ClanReference;
}

interface Room {
  vnum: number;
  title: string;
  description: string;
  zone: ZoneReference;
  exits: Exit[];
  flags: RoomFlag[];
  sector: SectorType;
  contents: Item[];
  occupants: Character[];
  position: WorldCoordinate;
}

interface Zone {
  vnum: number;
  name: string;
  description: string;
  level: LevelRange;
  resetMode: ResetMode;
  resetTime: number;
  rooms: Room[];
  mobs: Mobile[];
  objects: Item[];
  bounds: ZoneBounds;
}

interface Mobile {
  vnum: number;
  name: string;
  shortDesc: string;
  longDesc: string;
  level: number;
  stats: MobileStats;
  equipment: Equipment[];
  flags: MobileFlag[];
  triggers: Trigger[];
  location?: RoomReference;
}

interface Item {
  vnum: number;
  name: string;
  shortDesc: string;
  longDesc: string;
  type: ItemType;
  flags: ItemFlag[];
  values: ItemValues;
  location?: Location;
}
```

## API Design

### REST Endpoints

#### Character Management
```
GET    /api/characters              - List all characters
GET    /api/characters/:id          - Get character details
PUT    /api/characters/:id          - Update character
DELETE /api/characters/:id          - Delete character
POST   /api/characters/:id/actions  - Perform character actions (move, get, etc)
GET    /api/characters/:id/mail     - Get character mail
POST   /api/characters/:id/mail     - Send mail to character
```

#### World Management
```
GET    /api/zones                   - List all zones
GET    /api/zones/:vnum             - Get zone details
PUT    /api/zones/:vnum             - Update zone
POST   /api/zones                   - Create new zone
DELETE /api/zones/:vnum             - Delete zone

GET    /api/rooms                   - List/search rooms
GET    /api/rooms/:vnum             - Get room details
PUT    /api/rooms/:vnum             - Update room
POST   /api/rooms                   - Create new room
DELETE /api/rooms/:vnum             - Delete room

GET    /api/mobs                    - List/search mobs
GET    /api/mobs/:vnum              - Get mob details
PUT    /api/mobs/:vnum              - Update mob
POST   /api/mobs                    - Create new mob
DELETE /api/mobs/:vnum              - Delete mob

GET    /api/objects                 - List/search objects
GET    /api/objects/:vnum           - Get object details
PUT    /api/objects/:vnum           - Update object
POST   /api/objects                 - Create new object
DELETE /api/objects/:vnum           - Delete object
```

#### World Visualization
```
GET    /api/world/map               - Get world map data
GET    /api/world/graph/:zone       - Get zone connectivity graph
GET    /api/world/coordinates       - Get spatial positioning data
```

#### Real-time Events
```
WebSocket /ws/game-events           - Real-time game state updates
WebSocket /ws/world-changes         - World edit notifications
```

### API Middleware & Security
- **Authentication**: JWT tokens with role-based permissions
- **Rate Limiting**: 100 req/min for standard users, 1000 req/min for admins
- **Input Validation**: Zod schemas for all API inputs
- **Error Handling**: Standardized error responses with proper HTTP codes
- **Logging**: Structured logging with correlation IDs

## Frontend Architecture

### Component Structure
```
src/
├── components/
│   ├── character/
│   │   ├── CharacterList.tsx
│   │   ├── CharacterDetail.tsx
│   │   ├── CharacterInventory.tsx
│   │   └── CharacterMail.tsx
│   ├── world/
│   │   ├── ZoneList.tsx
│   │   ├── RoomEditor.tsx
│   │   ├── MobEditor.tsx
│   │   ├── ObjectEditor.tsx
│   │   └── WorldVisualization.tsx
│   ├── ui/
│   │   ├── Layout.tsx
│   │   ├── Navigation.tsx
│   │   ├── DataGrid.tsx
│   │   └── EditModal.tsx
│   └── common/
│       ├── ErrorBoundary.tsx
│       ├── LoadingSpinner.tsx
│       └── Toast.tsx
├── pages/
│   ├── Dashboard.tsx
│   ├── Characters.tsx
│   ├── World.tsx
│   └── Settings.tsx
├── hooks/
│   ├── useCharacters.ts
│   ├── useWorld.ts
│   └── useWebSocket.ts
├── stores/
│   ├── authStore.ts
│   ├── worldStore.ts
│   └── uiStore.ts
└── utils/
    ├── api.ts
    ├── validation.ts
    └── formatters.ts
```

### Key Features
- **Responsive Design**: Mobile-first approach with desktop enhancements
- **Real-time Updates**: Live data synchronization via WebSocket
- **Optimistic Updates**: Immediate UI feedback with rollback on error
- **Infinite Scrolling**: Efficient handling of large datasets
- **Search & Filtering**: Advanced search with faceted filtering
- **Bulk Operations**: Multi-select actions for efficiency
- **Undo/Redo**: Change history with rollback capabilities

## World Visualization System

### Map Types
1. **Zone Overview**: High-level zone layout with connections
2. **Room Network**: Detailed room-to-room connections within zones
3. **3D World View**: Immersive 3D representation of world spaces
4. **Minimap**: Quick navigation overlay

### Visualization Technologies
- **D3.js**: Data-driven room network graphs
- **React Flow**: Interactive node-based editing
- **Three.js**: 3D world visualization
- **Canvas API**: High-performance 2D rendering

### Visual Features
- **Interactive Navigation**: Click-to-navigate, zoom, pan
- **Real-time Updates**: Live position tracking of characters
- **Contextual Information**: Hover tooltips with room/mob/object details
- **Visual Indicators**: Color-coding for room types, danger levels, etc.
- **Path Finding**: Visual route planning between locations

## Data Synchronization

### File System Integration
```
MUD File Format → Parser → Database → API → Frontend
     ↓              ↓         ↓        ↓       ↓
  lib/world/    TypeScript  PostgreSQL REST   React
   *.wld         Parsers    Tables    JSON    Components
   *.mob
   *.obj
   *.zon
```

### Synchronization Strategy
- **Two-way Sync**: Web changes → Database → File system
- **Change Detection**: File watchers for external modifications
- **Conflict Resolution**: Last-write-wins with change history
- **Backup Strategy**: Automatic backups before modifications

## Security & Permissions

### Authentication System
- **Role-based Access**: Admin, Builder, Player, Guest
- **Permission Matrix**: Granular permissions per entity type
- **Session Management**: Secure JWT with refresh tokens
- **Audit Logging**: Complete change tracking

### Data Protection
- **Input Sanitization**: XSS and injection prevention
- **Rate Limiting**: API endpoint protection
- **CORS Configuration**: Secure cross-origin access
- **Data Validation**: Server-side validation for all operations

## Performance Considerations

### Backend Optimization
- **Database Indexing**: Optimized queries for common operations
- **Caching Strategy**: Redis for frequently accessed data
- **Connection Pooling**: Efficient database connections
- **Background Jobs**: Async file processing

### Frontend Optimization
- **Code Splitting**: Dynamic imports for route-based loading
- **Virtual Scrolling**: Efficient large list rendering
- **Image Optimization**: WebP with fallbacks
- **Bundle Analysis**: Webpack bundle optimization

## Implementation Phases

### Phase 1: Core Infrastructure (4-6 weeks)
- Backend API foundation
- Database schema and migrations
- Authentication system
- Basic frontend shell

### Phase 2: Character Management (3-4 weeks)
- Character viewing and editing
- Mail system integration
- Inventory management
- Real-time character tracking

### Phase 3: World Management (4-6 weeks)
- Zone/room/mob/object CRUD operations
- File system synchronization
- Basic world visualization
- Bulk import/export tools

### Phase 4: Advanced Features (6-8 weeks)
- 3D world visualization
- Advanced search and filtering
- Real-time collaboration
- Mobile optimization

## Deployment Architecture

### Development Environment
```
Frontend Dev Server (Vite) → Backend Dev Server → Local MUD Instance
Port 3000                     Port 3001          Port 4000
```

### Production Environment
```
Load Balancer → Web Server (Nginx) → API Servers → Database Cluster
                                                 → Redis Cluster
                                                 → MUD Server
```

### DevOps Pipeline
- **CI/CD**: GitHub Actions with automated testing
- **Containerization**: Docker for consistent deployments
- **Monitoring**: Application performance monitoring
- **Backup Strategy**: Automated database and file backups

## API Examples

### Character Data
```json
{
  "id": "char_123",
  "name": "Gandalf",
  "class": "sorcerer",
  "level": 50,
  "experience": 2500000,
  "stats": {
    "strength": 18,
    "intelligence": 25,
    "wisdom": 22,
    "constitution": 20,
    "charisma": 19,
    "dexterity": 16
  },
  "location": {
    "vnum": 3001,
    "title": "Temple of Midgaard",
    "zone": "Midgaard"
  },
  "online": true,
  "lastLogin": "2025-01-15T10:30:00Z"
}
```

### Room Data
```json
{
  "vnum": 3001,
  "title": "Temple of Midgaard",
  "description": "A magnificent temple with soaring columns...",
  "zone": {
    "vnum": 30,
    "name": "Midgaard"
  },
  "exits": [
    {"direction": "north", "to": 3002, "keywords": ["door"], "flags": []},
    {"direction": "south", "to": 3000, "keywords": [], "flags": []}
  ],
  "flags": ["sanctuary", "no_attack"],
  "sector": "inside",
  "coordinates": {"x": 0, "y": 0, "z": 0}
}
```

This design provides a comprehensive foundation for a modern MUD management interface while maintaining compatibility with the existing FieryMUD architecture.