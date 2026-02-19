# Scene API Reference

## Entity

UUID-based entity handle. Use `Entity::IsValid()` before accessing.

| Method | Description |
|--------|-------------|
| `bool IsValid()` | True if the entity exists in its scene. |
| `UUID GetUUID()` | Unique identifier. |
| `std::string GetName()` / `SetName(name)` | Display name. |
| `std::string GetTag()` / `SetTag(tag)` | Tag. |
| `bool IsActive()` / `SetActive(bool)` | Active state. |
| `bool HasComponent<T>()` | Check for a component. |
| `T& AddComponent<T>(args...)` | Add a component. |
| `T& GetComponent<T>()` | Get a component (throws if missing). |
| `void RemoveComponent<T>()` | Remove a component. |
| `Entity GetParent()` / `SetParent(entity)` | Hierarchy. |
| `std::vector<Entity> GetChildren()` | Children list. |

## Scene

| Method | Description |
|--------|-------------|
| `Entity CreateEntity(name)` | Create a new entity. |
| `void DestroyEntity(Entity e)` | Destroy an entity. |
| `bool HasEntity(UUID id)` | O(1) entity existence check. |
| `Entity GetEntityByUUID(UUID id)` | Look up entity by UUID. |
| `Entity GetEntityByName(name)` | O(1) lookup by name. |

## SceneSerializer

| Method | Description |
|--------|-------------|
| `void Serialize(path)` | Save scene to a `.scene` JSON file. |
| `void Deserialize(path)` | Load scene from a file. |
| `std::string SerializeToString()` | Serialize to JSON string. |
| `void DeserializeFromString(json)` | Load from JSON string. |
