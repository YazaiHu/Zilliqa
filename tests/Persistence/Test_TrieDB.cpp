/*
 * Copyright (C) 2019 Zilliqa
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <leveldb/db.h>
#include <string>

#include "depends/common/CommonIO.h"
#include "depends/common/FixedHash.h"
#include "depends/common/RLP.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include "depends/libDatabase/OverlayDB.h"
#pragma GCC diagnostic pop

#include "depends/libTrie/TrieDB.h"

using namespace std;
using namespace dev;

#define BOOST_TEST_MODULE persistencetest
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

template <class KeyType, class DB>
using SecureTrieDB = dev::SpecificTrieDB<dev::GenericTrieDB<DB>, KeyType>;

BOOST_AUTO_TEST_SUITE(persistencetest)

dev::h256 root1, root2;
// bytesConstRef k, k1;
string k1, k2;
h256 h;

BOOST_AUTO_TEST_CASE(createTwoTrieOnOneDB) {
  INIT_STDOUT_LOGGER();

  LOG_MARKER();

  dev::OverlayDB m_db("trieDB");
  m_db.ResetDB();

  SecureTrieDB<bytesConstRef, dev::OverlayDB> m_trie1(&m_db);
  m_trie1.init();

  k1 = "TestA";
  dev::RLPStream rlpStream1(2);
  rlpStream1 << "aaa"
             << "AAA";
  m_trie1.insert(k1, rlpStream1.out());
  k2 = "TestB";
  dev::RLPStream rlpStream2(2);
  rlpStream2 << "bbb"
             << "BBB";
  m_trie1.insert(k2, rlpStream2.out());
  m_trie1.db()->commit();
  root1 = m_trie1.root();
  LOG_GENERAL(INFO, "root1 = " << root1);

  for (auto i : m_trie1) {
    dev::RLP rlp(i.second);
    LOG_GENERAL(INFO, "ITERATE k: " << i.first.toString()
                                    << " v: " << rlp[0].toString() << " "
                                    << rlp[1].toString());
  }

  BOOST_CHECK_MESSAGE(m_trie1.contains(k1),
                      "ERROR: Trie1 cannot get the element in Trie1");

  SecureTrieDB<h256, dev::OverlayDB> m_trie2(&m_db);
  m_trie2.init();
  h = dev::h256::random();
  m_trie2.insert(h, string("hhh"));

  m_trie2.db()->commit();
  root2 = m_trie2.root();
  LOG_GENERAL(INFO, "root2 = " << root2);
  LOG_GENERAL(INFO, "h: " << h << " v: " << m_trie2.at(h));

  BOOST_CHECK_MESSAGE(m_trie2.contains(h),
                      "ERROR: Trie2 cannot get the element in Trie2");

  // Test Rollback
  h256 t = dev::h256::random();
  m_trie2.insert(t, string("ttt"));
  BOOST_CHECK_MESSAGE(m_trie2.contains(t),
                      "ERROR: Trie2 cannot get the element not committed");
  BOOST_CHECK_MESSAGE(
      root2 != m_trie2.root(),
      "ERROR, Trie2 still has the same root after insert and before commit");
  m_trie2.db()->rollback();
  BOOST_CHECK_MESSAGE(!m_trie2.contains(t),
                      "ERROR: Trie2 still have the new element after rollback");
  m_trie2.setRoot(root2);
  BOOST_CHECK_MESSAGE(m_trie2.contains(h),
                      "ERROR: Trie2 still cannot get the the old element "
                      "after reset the root to the old one");
}

BOOST_AUTO_TEST_CASE(retrieveDataStoredInTheTwoTrie) {
  dev::OverlayDB m_db("trieDB");
  SecureTrieDB<bytesConstRef, dev::OverlayDB> m_trie3(&m_db);
  SecureTrieDB<h256, dev::OverlayDB> m_trie4(&m_db);
  m_trie3.setRoot(root1);
  m_trie4.setRoot(root2);

  BOOST_CHECK_MESSAGE(m_trie3.contains(k1),
                      "ERROR: Trie3 cannot get the element in Trie1");
  BOOST_CHECK_MESSAGE(m_trie4.contains(h),
                      "ERROR: Trie4 cannot get the element in Trie2");
}

BOOST_AUTO_TEST_SUITE_END()
