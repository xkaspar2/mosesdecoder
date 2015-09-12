// -*- c++ -*-
// $Id$

/***********************************************************************
Moses - factored phrase-based language decoder
Copyright (C) 2006 University of Edinburgh

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
***********************************************************************/

#ifndef moses_PhraseDictionary_h
#define moses_PhraseDictionary_h

#include <iostream>
#include <map>
#include <memory>
#include <list>
#include <stdexcept>
#include <vector>
#include <string>
#include <boost/unordered_map.hpp>

#ifdef WITH_THREADS
#include <boost/thread/tss.hpp>
#include "moses/ThreadPool.h"
#else
#include <boost/scoped_ptr.hpp>
#include <ctime>
#endif

#include "moses/Phrase.h"
#include "moses/TargetPhrase.h"
#include "moses/TargetPhraseCollection.h"
#include "moses/InputPath.h"
#include "moses/FF/DecodeFeature.h"

namespace Moses
{

class StaticData;
class InputType;
class WordsRange;
class ChartCellCollectionBase;
class ChartRuleLookupManager;
class ChartParser;

class CacheColl : public boost::unordered_map<size_t, std::pair<const TargetPhraseCollection*, clock_t> >
{
// 1st = hash of source phrase/ address of phrase-table node
// 2nd = all translations
// 3rd = time of last access

public:
  ~CacheColl();
};

/**
  * Abstract base class for phrase dictionaries (tables).
  **/
class PhraseDictionary :  public DecodeFeature
{
protected:
  class LookupTask: public Task
  {
  public:
    LookupTask(const PhraseDictionary *phraseDictionary, const Phrase &phrase, const TargetPhraseCollection **targetPhrases) : m_phrase(phrase), m_phraseDictionary(phraseDictionary), m_targetPhrases(targetPhrases) {}
    void Run();
  private:
    const Phrase &m_phrase;
    const PhraseDictionary *m_phraseDictionary;
    const TargetPhraseCollection **m_targetPhrases;
  };

private:
  friend class PhraseDictionaryMultiModelCounts;
  // why is this necessary? that's a derived class, so it should have
  // access to the
public:
  virtual bool ProvidesPrefixCheck() const;

  static const std::vector<PhraseDictionary*>& GetColl() {
    return s_staticColl;
  }

  PhraseDictionary(const std::string &line, bool registerNow);

  virtual ~PhraseDictionary() {
  }

  //! table limit number.
  size_t GetTableLimit() const {
    return m_tableLimit;
  }

  //! continguous id for each pt, starting from 0
  size_t GetId() const {
    return m_id;
  }

  virtual
  void
  Release(TargetPhraseCollection const* tpc) const;

  /// return true if phrase table entries starting with /phrase/
  //  exist in the table.
  virtual
  bool
  PrefixExists(ttasksptr const& ttask, Phrase const& phrase) const;

  // LEGACY!
  // The preferred method is to override GetTargetPhraseCollectionBatch().
  // See class PhraseDictionaryMemory or PhraseDictionaryOnDisk for details
  //! find list of translations that can translates src. Only for phrase input

public:
  virtual
  TargetPhraseCollection const *
  GetTargetPhraseCollectionLEGACY(const Phrase& src) const;

  virtual
  TargetPhraseCollection const *
  GetTargetPhraseCollectionLEGACY(ttasksptr const& ttask, const Phrase& src) const {
    return GetTargetPhraseCollectionLEGACY(src);
  }

  virtual
  void
  GetTargetPhraseCollectionBatch(const InputPathList &inputPathQueue) const;

  virtual
  void
  GetTargetPhraseCollectionBatch(ttasksptr const& ttask,
                                 const InputPathList &inputPathQueue) const {
    GetTargetPhraseCollectionBatch(inputPathQueue);
  }

  //! Create entry for translation of source to targetPhrase
  virtual void InitializeForInput(ttasksptr const& ttask) {
  }
  // clean up temporary memory, called after processing each sentence
  virtual void CleanUpAfterSentenceProcessing(const InputType& source) {
  }

  //! Create a sentence-specific manager for SCFG rule lookup.
  virtual ChartRuleLookupManager *CreateRuleLookupManager(
    const ChartParser &,
    const ChartCellCollectionBase &,
    std::size_t) = 0;

  const std::string &GetFilePath() const {
    return m_filePath;
  }

  const std::vector<FeatureFunction*> &GetFeaturesToApply() const {
    return m_featuresToApply;
  }

  void SetParameter(const std::string& key, const std::string& value);

  // LEGACY
  //! find list of translations that can translates a portion of src. Used by confusion network decoding
  virtual const TargetPhraseCollectionWithSourcePhrase* GetTargetPhraseCollectionLEGACY(InputType const& src,WordsRange const& range) const;

protected:
  static std::vector<PhraseDictionary*> s_staticColl;

  size_t m_tableLimit;
  std::string m_filePath;

  // features to apply evaluate target phrase when loading.
  // NOT when creating translation options. Those are in DecodeStep
  std::vector<FeatureFunction*> m_featuresToApply;

  // MUST be called at the start of Load()
  void SetFeaturesToApply();

  bool SatisfyBackoff(const InputPath &inputPath) const;

  // cache
  size_t m_maxCacheSize; // 0 = no caching

#ifdef WITH_THREADS
  //reader-writer lock
  mutable boost::thread_specific_ptr<CacheColl> m_cache;
#else
  mutable boost::scoped_ptr<CacheColl> m_cache;
#endif

  virtual const TargetPhraseCollection *GetTargetPhraseCollectionNonCacheLEGACY(const Phrase& src) const;
  void ReduceCache() const;

protected:
  CacheColl &GetCache() const;
  size_t m_id;

  size_t m_maxPhraseLength;
#ifdef WITH_THREADS
  size_t m_maxParallelQueries;
  boost::shared_ptr<ThreadPool> m_queryThreadPool; //to let the PhraseTable be constant (and maybe later ThreadPool to be shared)
#endif
};

}
#endif
