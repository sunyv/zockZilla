
#include "../include/CStatistics.h"
#include "../include/CUnitCounterFactory.h"
#include "../include/CMatcher.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/filesystem.hpp>

#include <exception>
#include <sstream>
#include <chrono>
#include <map>
#include <string>

namespace GSC
{
namespace Common
{
   namespace pt = boost::property_tree;
   namespace fs = boost::filesystem;
   namespace ch = std::chrono;
   
   namespace
   {
      typedef std::map< std::string, std::unique_ptr<API::IUnitCounter> > CounterMapType;
   }
   
   struct Statistics::Impl
   {
      Impl( log4cxx::LoggerPtr logger, boost::filesystem::path const& path ) : 
          m_logger( logger )
         ,m_path( path )
         ,m_now( ch::system_clock::now() )
         ,m_lastRun()
         ,m_counter()
      {}
      
      log4cxx::LoggerPtr m_logger;
      boost::filesystem::path m_path;
      std::chrono::system_clock::time_point m_now;
      std::chrono::system_clock::time_point m_lastRun;
      CounterMapType m_counter;      
   };
   
   Statistics::Statistics( log4cxx::LoggerPtr logger, boost::filesystem::path const& path ) : 
       m_impl( new Impl( logger, path ) )
   {
      if ( !fs::exists( m_impl->m_path ) )
      {
         fs::create_directories( m_impl->m_path.parent_path() );
         save(); ///< Save initial statistics file
      }
      else
      {
         if ( !fs::is_regular_file( m_impl->m_path ) )
         {  throw std::invalid_argument( std::string( "Path exists but is not a file: " ) + m_impl->m_path.string() ); }
      }
      load();      
   }
   
   Statistics::~Statistics()
   {
      try
      {  save(); }
      catch ( std::exception const& e )
      {  LOG4CXX_ERROR( m_impl->m_logger, e.what() ); }
   }
   
   API::IRatedMatch::SetType Statistics::rate(API::IMatch::SetType const& matches)
   {
      API::IRatedMatch::SetType result;
      for (auto const& match : matches)
      {
         auto& counter( m_impl->m_counter[match->getName()] );
         if (!counter)
         {
            /** \todo When the counter is new and created the first time,
             *  we pass here some standard values Unit:Day
             *  but this is invalid, this has to come from configuration
             *  file.
             */
            counter = CUnitCounterFactory().create(API::Unit::Day);
         }
      
         counter->doUpdate(m_impl->m_now);
         result.emplace(std::make_unique<CRatedMatch>(
             *match
            ,counter->exceedsLimit(match->getRule().getLimit())));
      }
      return result;
   }
   
   void Statistics::save() const
   {
      pt::ptree tree;
      tree.put( "statistics.lastRun", m_impl->m_now.time_since_epoch().count() );
      for ( auto const& counter : m_impl->m_counter )
      {
         pt::ptree ptCounter(counter.second->serialize());
         ptCounter.put( "name", counter.first );
         tree.add_child( "statistics.counters.counter", ptCounter);
      }
      pt::write_xml( m_impl->m_path.string(), tree, std::locale(), pt::xml_writer_make_settings<std::string>( ' ', 2 ) );
   }
   
   void Statistics::load()
   {
      pt::ptree tree;
      pt::read_xml( m_impl->m_path.string(), tree, pt::xml_parser::trim_whitespace );
      m_impl->m_lastRun = ch::system_clock::time_point( ch::system_clock::time_point::duration( tree.get( "statistics.lastRun", m_impl->m_now.time_since_epoch().count() ) ) );
      boost::optional< pt::ptree& > ptCounters( tree.get_child_optional( "statistics.counters" ) );
      if ( ptCounters )
      {
         for ( auto const& child : ptCounters->get_child( "" ) )
         {
            pt::ptree const& ptCounter( child.second );
            std::string const name( ptCounter.get< std::string >( "name" ) );
            m_impl->m_counter.emplace(CounterMapType::value_type(name, CUnitCounterFactory().create(ptCounter)));
         }
      }
   }

}}
