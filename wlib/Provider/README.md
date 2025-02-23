# WLib SPI Interface

## Motivation

## Interface

### `Connection_Interface`

  `WLib::SPI::Connection_Interface` stellt das abstrakte Interface für eine SPI Verbindung dar.
  Mit `transcieve` können Daten zum/vom SPI-Peripheral gesendet/empfangen werden.

  #### Usecases:

  ```cpp
  std::byte const tx[N] = {/***/};
  std::byte rx[N];
  std::byte txrx[N];
  ```

  ```cpp
  // senden aus dem tx-buffer, empfange bytes landen im rx-buffer
  connection.transcieve(tx, rx, N);
  
  // senden aus dem txrx-buffer, empfange bytes landen ebenfalls im txrx-buffer
  connection.transcieve(txrx, txrx, N);
  
  // senden von N aus dem tx-buffer, empfange bytes werden verworfen
  connection.transcieve(tx, nullptr, N);
  
  // empfangen von N bytes landen im rx-buffer
  connection.transcieve(nullptr, rx, N);
  
  // takten von N byte
  connection.transcieve(nullptr, nullptr, N);
  ```

  ```cpp
  class Connection_Interface
  {
  public:
    virtual ~Connection_Interface() = default;
    virtual void transcieve(std::byte const* tx, std::byte* rx, std::size_t const& len) = 0;
  };
  ```

### `Hardware_Interface`

```cpp
class Hardware_Interface
    : protected Connection_Interface
    , private Internal::no_copy_no_move
{
public:
  virtual ~Hardware_Interface() = default;

protected:
  static Hardware_Interface& get_dummy();
  virtual void               enable(cfg_t const&) = 0;
  virtual void               disable()            = 0;
  // ...
};
```

```cpp
class Chipselect_Interface : private Internal::no_copy_no_move
{
public:
  virtual ~Chipselect_Interface() = default;

protected:
  virtual void select()   = 0;
  virtual void deselect() = 0;
  // ...
};
```

## Handles

```cpp
class Connection_handle_t final
     : public Connection_Interface
     , private Internal::no_copy_no_move
{
public:
  Connection_handle_t(Hardware_Interface& hw, Chipselect_Interface& cs, Connection_Interface& con);
  ~Connection_handle_t();
  
  void transcieve(std::byte const* tx, std::byte* rx, std::size_t const& len) override;
  // ...
};
```

```cpp
class Channel_handle_t final : private Internal::no_copy_no_move
{
public:
  Channel_handle_t(Hardware_Interface& hw, Chipselect_Interface& cs, Hardware_Interface::cfg_t const&cfg);
  ~Channel_handle_t();
  Connection_handle_t select() &;
  Connection_handle_t select() &&;
 // ...
};
```

```cpp
class Hardware_handle_t final : private Internal::no_copy_no_move
{
public:
  Hardware_handle_t(Hardware_Interface& hw, Hardware_Interface::cfg_t const& cfg);
  ~Hardware_handle_t();
  Connection_handle_t select(Chipselect_Interface& cs) &;
  Connection_handle_t select(Chipselect_Interface& cs) &&;
  // ...
};
```

## Example

```cpp
```

```cpp
```

```cpp
```

```cpp
```
