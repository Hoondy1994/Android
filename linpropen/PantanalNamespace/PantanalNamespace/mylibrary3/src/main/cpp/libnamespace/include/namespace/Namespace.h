//
// Created by 80244960 on 2022/12/7.
//

#ifndef NAMESPACE_NAMESPACE_H
#define NAMESPACE_NAMESPACE_H

#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * init Ubiquitous service namespace environment
 * @param id Ubiquitous service id
 * @param data namespace data
 * @return 0 if succeed
 */
int ns_client_add(uint64_t id, void *data);

/**
 * remove Ubiquitous service namespace environment
 * @param id Ubiquitous service id
 * @return 0 if succeed
 */
int ns_client_remove(uint64_t id);

#ifdef __cplusplus
}
#endif

#endif //NAMESPACE_NAMESPACE_H
