#include <wlib-HASH.hpp>

namespace wlib::hash
{

  uint32_t&       sha_256::internal_state_t::operator[](uint32_t idx) noexcept { return this->m_value[idx]; }
  
  uint32_t const& sha_256::internal_state_t::operator[](uint32_t idx) const noexcept { return this->m_value[idx]; }

  sha_256::internal_state_t& sha_256::internal_state_t::operator+=(internal_state_t const& rhs) noexcept
  {
    for (uint32_t i = 0; i < this->m_value.size(); ++i)
    {
      this->m_value[i] += rhs[i];
    }
    return *this;
  }

  sha_256::internal_state_t sha_256::internal_state_t::operator+(internal_state_t const& rhs) const noexcept
  {
    internal_state_t ret = *this;
    ret += rhs;
    return ret;
  }

  sha_256::hash_t sha_256::internal_state_t::to_hash() const noexcept
  {
    hash_t ret{};
    for (uint32_t i = 0; i < this->m_value.size(); ++i)
    {
      ret[i * 4 + 0] = std::byte((this->m_value[i] >> 24) & 0xFF);
      ret[i * 4 + 1] = std::byte((this->m_value[i] >> 16) & 0xFF);
      ret[i * 4 + 2] = std::byte((this->m_value[i] >> 8) & 0xFF);
      ret[i * 4 + 3] = std::byte((this->m_value[i] >> 0) & 0xFF);
    }
    return ret;
  }

  sha_256& sha_256::operator()(std::span<std::byte const> const& data) noexcept
  {
    for (std::byte const& cur : data)
    {
      this->m_blk[this->m_idx++] = cur;
      this->m_len += 8;
      if (this->m_idx < 64)
        continue;

      this->m_idx            = 0;
      this->m_internal_state = this->process_blk(this->m_internal_state, this->m_blk);
    }
    return *this;
  }

  void sha_256::reset() noexcept
  {
    this->m_len            = 0;
    this->m_idx            = 0;
    this->m_blk            = {};
    this->m_internal_state = {};
  }

  sha_256::hash_t sha_256::get() const noexcept
  {
    auto     tmp_blk = this->m_blk;
    uint32_t tmp_idx = this->m_idx;

    internal_state_t internal_state = this->m_internal_state;

    if (tmp_idx > 64 - 9)
    {
      tmp_blk[tmp_idx++] = std::byte(0x80);
      for (; tmp_idx < 64; tmp_idx++)
        tmp_blk[tmp_idx] = std::byte(0);
      tmp_idx        = 0;
      internal_state = this->process_blk(internal_state, tmp_blk);
    }
    else
    {
      tmp_blk[tmp_idx++] = std::byte(0x80);
    }

    for (; tmp_idx < 56; tmp_idx++)
      tmp_blk[tmp_idx] = std::byte(0);

    tmp_blk[56]    = std::byte((this->m_len >> 56) & 0xFF);
    tmp_blk[57]    = std::byte((this->m_len >> 48) & 0xFF);
    tmp_blk[58]    = std::byte((this->m_len >> 40) & 0xFF);
    tmp_blk[59]    = std::byte((this->m_len >> 32) & 0xFF);
    tmp_blk[60]    = std::byte((this->m_len >> 24) & 0xFF);
    tmp_blk[61]    = std::byte((this->m_len >> 16) & 0xFF);
    tmp_blk[62]    = std::byte((this->m_len >> 8) & 0xFF);
    tmp_blk[63]    = std::byte((this->m_len >> 0) & 0xFF);
    internal_state = this->process_blk(internal_state, tmp_blk);

    return internal_state.to_hash();
  }

  sha_256::internal_state_t sha_256::process_blk(internal_state_t state, chunk_t const& blk) noexcept
  {
    static constexpr uint32_t k[64] = { 0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01,
                                        0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
                                        0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da, 0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147,
                                        0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
                                        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070, 0x19a4c116, 0x1e376c08,
                                        0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
                                        0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2 };

    uint32_t w[64];
    for (int i = 0; i < 16; ++i)
    {
      w[i] = (static_cast<uint32_t>(blk[i * 4 + 0]) << 24) | (static_cast<uint32_t>(blk[i * 4 + 1]) << 16) | (static_cast<uint32_t>(blk[i * 4 + 2]) << 8) |
             static_cast<uint32_t>(blk[i * 4 + 3]);
    }
    for (int i = 16; i < 64; ++i)
    {
      uint32_t s0 = std::rotr(w[i - 15], 7) ^ std::rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
      uint32_t s1 = std::rotr(w[i - 2], 17) ^ std::rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
      w[i]        = w[i - 16] + s0 + w[i - 7] + s1;
    }

    internal_state_t tmp = state;
    for (uint64_t i = 0; i < 64; ++i)
    {
      uint32_t S1    = std::rotr(tmp[4], 6) ^ std::rotr(tmp[4], 11) ^ std::rotr(tmp[4], 25);
      uint32_t ch    = (tmp[4] & tmp[5]) ^ (~tmp[4] & tmp[6]);
      uint32_t temp1 = tmp[7] + S1 + ch + k[i] + w[i];
      uint32_t S0    = std::rotr(tmp[0], 2) ^ std::rotr(tmp[0], 13) ^ std::rotr(tmp[0], 22);
      uint32_t maj   = (tmp[0] & tmp[1]) ^ (tmp[0] & tmp[2]) ^ (tmp[1] & tmp[2]);
      uint32_t temp2 = S0 + maj;

      tmp[7] = tmp[6];
      tmp[6] = tmp[5];
      tmp[5] = tmp[4];
      tmp[4] = tmp[3] + temp1;
      tmp[3] = tmp[2];
      tmp[2] = tmp[1];
      tmp[1] = tmp[0];
      tmp[0] = temp1 + temp2;
    }

    state += tmp;
    return state;
  }
}    // namespace wlib::hash
