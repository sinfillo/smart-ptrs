# smart-ptrs

Реализация умных указателей из С++. Реализация `UniquePtr` (аналог `std::unique_ptr` из C++) лежит в директории `unique`, реализация  `SharedPtr` (`std::shared_ptr` в C++), `WeakPtr` (`std::weak_ptr`) и `SharedFromThis` (`std::enable_shared_from_this`) лежит в директории `shared`. Также был реализован  `IntrusivePtr` -- умный указатель, похожий по семантике на `SharedPtr`, без возможности брать `WeakPtr` на указатель. Особенность этого указателя: счетчик ссылок находится прямо в объекте.
