//
// Created by shane on 12/02/2021.
//

#ifndef RX_UTIL_H
#define RX_UTIL_H

#define RX_NO_COPY_NO_MOVE(className)   className(const className & other) = delete; \
                                        className(className && other) noexcept = delete; \
                                        className & operator=(const className & other) = delete; \
                                        className & operator=(className && other) noexcept = delete;

#endif //RX_UTIL_H
