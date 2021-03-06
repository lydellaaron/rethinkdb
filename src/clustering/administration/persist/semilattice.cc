// Copyright 2010-2015 RethinkDB, all rights reserved.
#include "clustering/administration/persist/semilattice.hpp"

#include "clustering/administration/metadata.hpp"
#include "concurrency/wait_any.hpp"

template <class metadata_t>
semilattice_persister_t<metadata_t>::semilattice_persister_t(
        metadata_file_t *f,
        const metadata_file_t::key_t<metadata_t> &k,
        std::shared_ptr<semilattice_read_view_t<metadata_t> > v) :
    file(f), key(k), view(v),
    persist_pumper(std::bind(&semilattice_persister_t::persist, this, ph::_1)),
    subs(std::bind(&pump_coro_t::notify, &persist_pumper), v)
{
    persist_pumper.notify();
}

template <class metadata_t>
void semilattice_persister_t<metadata_t>::persist(signal_t *interruptor) {
    if (interruptor->is_pulsed()) {
        throw interrupted_exc_t();
    }
    cond_t non_interruptor;
    metadata_file_t::write_txn_t txn(file, &non_interruptor);
    txn.write(key, view->get(), &non_interruptor);
    txn.commit();
}

template class semilattice_persister_t<cluster_semilattice_metadata_t>;
template class semilattice_persister_t<auth_semilattice_metadata_t>;
template class semilattice_persister_t<heartbeat_semilattice_metadata_t>;
