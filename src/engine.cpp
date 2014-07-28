#include "types.h"

#include <algorithm>
#include <cassert>
#include <cfloat>
#include <cmath>
#include <cstring>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <map>


#include "movegen.h"
#include "notation.h"
#include "search.h"
#include "thread.h"
#include "engine.h"
#include "dataaccess.h"
#include "bitcount.h"
#include "rkiss.h"
#include "position.h"
#include "bitboard.h"

namespace {
    //GENERATION
    DataAccess::PieceStatistics* pss[THEME_NB][PIECE_NB];
    
    //STATISTICS
    int ilegals_fen = 0;
    int legals_mi2 = 0;
    int ilegals = 0;
    int not_mi2 = 0;
    int score_less=0;
    int gen_src=0;
    
    int db_num = 0;
    
    //IMPROVMENT STATS
    //uint64_t nodes[4];
    
    //Transformtion deltas
    Square move_deltas[4] = {DELTA_N, DELTA_S, DELTA_E, DELTA_W};
}

static int check_yacp_callback(void *data, int argc, char **argv,
        char **azColName)
{    
    std::string fen = std::string(argv[7]);
    
    if (fen.find('(') != std::string::npos
            || fen.find(')') != std::string::npos
            || fen.find('k') == std::string::npos
            || fen.find('K') == std::string::npos
            || fen.find('x') != std::string::npos
            || fen.find('X') != std::string::npos
            || fen.find('o') != std::string::npos
            || fen.find('O') != std::string::npos
            || fen.find('e') != std::string::npos
            || fen.find('E') != std::string::npos
            || fen.find('f') != std::string::npos
            || fen.find('F') != std::string::npos)
    {
        //std::cout << argv[0] << std::endl;
        //std::cout << "illegal fen format" << std::endl;
        ilegals_fen++;
        return 0;
    }
    
   Position pos = Position(std::string(argv[7]) + " w - - 0 1");
   if (!pos.pos_is_ok())
   {
       ilegals++;
       return 0;
   }
   
   Search::Composition compos = Search::Composition(pos, 3);
   
   if (!compos.legal_mate_in_2())
   {
       not_mi2++;
       return 0;
   }
   
   legals_mi2++;
   return 0;
}

static int print_ps_callback(void *data, int argc, char **argv, char **azColName)
{
    DataAccess::PieceStatistics ps((Piece)atoi(argv[1]));    
    ps.total = atoi(argv[2]);
    
    for (Square s=SQ_A1; s<=SQ_H8; ++s)
    {
        ps.squares[s] = atoi(argv[s+3]);
    }
        
    for (int i=0; i<9; ++i)
    {
        if (argc <= i+67)
            std::cout << "problematic i: " << i << ". i+67=" << i+67 << std::endl;
        ps.amount[i] = atoi(argv[i+67]);
    }
    
    std::cout << ps.toString() << std::endl;
    
    return 0;
}

static int genstat_callback(void *score, int argc, char **argv, char **azColName)
{
    //std::cout << "ID: " << argv[0] << std::endl;
    
    
    std::string fen = std::string(argv[7]);
    
    if (fen.find('(') != std::string::npos
            || fen.find(')') != std::string::npos
            || fen.find('k') == std::string::npos
            || fen.find('K') == std::string::npos
            || fen.find('x') != std::string::npos
            || fen.find('X') != std::string::npos
            || fen.find('o') != std::string::npos
            || fen.find('O') != std::string::npos
            || fen.find('e') != std::string::npos
            || fen.find('E') != std::string::npos
            || fen.find('f') != std::string::npos
            || fen.find('F') != std::string::npos)
    {
        ilegals_fen++;
        return 0;
    }
    
   Position pos = Position(std::string(argv[7]) + " w - - 0 1");
   if (!pos.pos_is_ok())
   {
       ilegals++;
       return 0;
   }
   
   Search::Composition compos = Search::Composition(pos, 3);
   
   if (!compos.legal_mate_in_2())
   {
       not_mi2++;
       return 0;
   }
   
   legals_mi2++;
   
   std::string id = std::string(argv[0]);
   
   //std::cout <<
   compos.calc_score();
   
   // If get by score and current compos score is less then needed - exit
   if ((double*)score && compos.score < *((double*)score))
   {
       score_less++;
       return 0;
   }
   
   gen_src++;
   
   for (Theme t=GRIMSHAW; t!=THEME_NB; ++t)
   {
       if (!(compos.themes & t))
           continue;
       
        //if (t==KING_FLIGHTS && compos.themes & KING_FLIGHTS)
        //   std::cout << "king flights found!" << std::endl;       
       //std::cout << "theme " << t << std::endl;
       
       for (Color c=WHITE; c<=BLACK; ++c)
           for (PieceType pt=PAWN; pt<=KING; ++pt)
           {
               Piece p = make_piece(c,pt);
               //if not initialized
               if (!pss[t][p])
                    pss[t][p] = new DataAccess::PieceStatistics(p);

               //std::cout << p << std::endl;
               Bitboard b = pos.pieces(c, pt);
               int num_p = popcount<Full>(b);

               pss[t][p]->total += num_p;
               pss[t][p]->amount[num_p]++;
               
               while(b)
               {
                   pss[t][p]->squares[pop_lsb(&b)]++;
               }
           }
   }
   
   return 0;
}

static int load_callback(void *theme, int argc, char **argv, char **azColName)
{
    DataAccess::PieceStatistics* ps = new DataAccess::PieceStatistics((Piece)atoi(argv[1]));    
    ps->total = atoi(argv[2]);
    
    for (Square s=SQ_A1; s<=SQ_H8; ++s)
    {
        ps->squares[s] = atoi(argv[s+3]);
    }
        
    for (int i=0; i<9; ++i)
    {
        ps->amount[i] = atoi(argv[i+67]);
    }
    
    pss[*(Theme*)theme][(Piece)atoi(argv[1])] = ps;
    
    return 0;
}


void Engine::generate_stats(double* score_p)
{
    time_t gen_start, gen_end;
    char gen_tm[80];
    
    time (&gen_start);
    
    Time::point msec = Time::now();
    
    DataAccess::open_db(CLASSIC);
    DataAccess::open_db(CREATED);
    
    DataAccess::getAllCmp(CLASSIC, genstat_callback, score_p);
    
    for (Theme t=GRIMSHAW; t!=THEME_NB; ++t)
        for (Piece p=W_PAWN; p!=PIECE_NB; ++p)
            if(pss[t][p])
            {
                DataAccess::addPS(*pss[t][p], t);
                delete pss[t][p];
            }
    
    DataAccess::close_db(CREATED);
    DataAccess::close_db(CLASSIC);
    
    time (&gen_end);
    gen_end -= gen_start;
    strftime (gen_tm,80,"%T",gmtime(&gen_end));
    
    msec = Time::now() - msec;
    
    std::cout << "Stats. NOT LEGAL FEN: " << ilegals_fen
            << ". NOT LEGAL: " << ilegals
            << ". NOT MATE IN 2: " << not_mi2
            << ". LEGAL MATE IN 2: " << legals_mi2
            << ". SCORE LESS: " << score_less
            << ". GENERATION SOURCE: " << gen_src << std::endl
            << "Time: " << gen_tm << ". "
            << (double)(ilegals+not_mi2+legals_mi2) / msec
            << " pos./msec."<< std::endl;
}

void Engine::generate_compos(Theme t, Time::point dur, bool debug)
{
    DataAccess::open_db(CREATED);

    // probabilities and cumulative probabilities for #pieces and squares
    std::multimap<double,int> p_amount[PIECE_NB], p_squares[PIECE_NB],
            cp_amount[PIECE_NB], cp_squares[PIECE_NB];
    double am_limits[PIECE_NB], sq_limits[PIECE_NB];
    
    // Possible squares for each PieceType by statistics
    Bitboard stat_sqrs[PIECE_NB];

    // if not SCRATCH
    if (t)
    {
        DataAccess::getPSByTheme(t, load_callback);
        assert(pss[t]);



        //Calc probability for each square
        for (Color c=WHITE; c<=BLACK; ++c)
            for (PieceType pt=PAWN; pt<=KING; ++pt)
            {
                Piece p = make_piece(c,pt);
                assert(pss[t][p]);

                //std::cout << "\n\n" << p << " TOTAL: " << pss[t][p]->total << std::endl;
                for (int i=0; i<=8; ++i)
                {
                    //pss[t][W_KING]->total - total number of positions from theme t
                    p_amount[p].insert(std::pair<double,int>(
                            (double)pss[t][p]->amount[i]/pss[t][W_KING]->total,i));
                }

                stat_sqrs[p] = 0;

                for (Square i=SQ_A1; i<=SQ_H8; ++i)
                {
                    if(pss[t][p]->squares[i])
                        stat_sqrs[p] |= i; 

                    p_squares[p].insert(std::pair<double,int>(
                            (double)pss[t][p]->squares[i]/pss[t][p]->total,i));
                }
            }

        //Calc Cumulative probability
        for (Color c=WHITE; c<=BLACK; ++c)
            for (PieceType pt=PAWN; pt<=KING; ++pt)
            {
                double cp = 0.0;
                Piece p = make_piece(c,pt);

                for (std::map<double,int>::iterator it=p_amount[p].begin();
                        it!=p_amount[p].end(); ++it)
                {
                    cp_amount[p].insert(std::pair<double,int>(it->first + cp, it->second));
                    cp += it->first;
                }

                am_limits[p] = cp;

                cp = 0.0;
                for (std::map<double,int>::iterator it=p_squares[p].begin();
                        it!=p_squares[p].end(); ++it)
                {
                    cp_squares[p].insert(std::pair<double,int>(it->first + cp, it->second));
                    cp += it->first;
                }

                sq_limits[p] = cp;
            }

        for (Color c=WHITE; c<=BLACK; ++c)
            for (PieceType pt=PAWN; pt<=KING; ++pt)
            {
                Piece p = make_piece(c,pt);

                if(debug) 
                {
                    std::cout << "\n\n" << p << " amount:" << std::endl;
                    /*for (std::map<double,int>::iterator mit=p_amount[p].begin();
                            mit!=p_amount[p].end(); ++mit)
                        std::cout << mit->first << ": " << mit->second << std::endl;
                    */

                    for (std::map<double,int>::iterator mit=cp_amount[p].begin();
                            mit!=cp_amount[p].end(); ++mit)
                        std::cout << mit->first << ": " << mit->second << std::endl;

                    std::cout << "amount limit: " << am_limits[p] << std::endl;

                    std::cout << "\n" << p << " squares:" << std::endl;
                    /*for (std::map<double,int>::iterator mit=p_squares[p].begin();
                            mit!=p_squares[p].end(); ++mit)
                        std::cout << mit->first << ": " << mit->second << std::endl;   
                    */
                    std::cout << std::endl;

                    std::cout << Bitboards::pretty(stat_sqrs[p]) << std::endl;

                    for (std::map<double,int>::iterator mit=cp_squares[p].begin();
                            mit!=cp_squares[p].end(); ++mit)
                        std::cout << mit->first << ": " << mit->second << std::endl;  

                    std::cout << "squares limit: " << sq_limits[p] << std::endl;
                }
            }

    }
    //MAYDO:: then when generating use binary_search algotith to find needed hiztabrut mztaberet
    
            
    /*for (Color c=WHITE; c<=BLACK; ++c)    
        for (PieceType pt=PAWN; pt<=KING; ++pt)    
        {
            Piece p = make_piece(c,pt);
            std::cout << p << ":\n" << Bitboards::pretty(stat_sqrs[p])<<std::endl;
        }
    */
    
    // Generated compositions with score >= 50    
    std::vector<DataAccess::GenCompos> generated;    
    
    int gen_stats[GCT_NB];
    memset(gen_stats,0,4*GCT_NB);
    gen_stats[GC_THEME_ID] = t;
    //std::cout << UINT32_MAX << std::endl;
    
    
    int themes_nb[THEMES_MAX];
    int bonuses_nb[BONUSES_MAX];
    int penalties_nb[PENALTIES_MAX];
    
    memset(themes_nb, 0, sizeof(int)*THEMES_MAX);
    memset(bonuses_nb, 0, sizeof(int)*BONUSES_MAX);
    memset(penalties_nb, 0, sizeof(int)*PENALTIES_MAX);
    
    srand (time(NULL));
    Position pos = Position("4k3/8/8/8/8/8/8/4K3 w - - 0 1");

    //DataAccess::clearGS();
    
    // total_time and total_nodes of last generated compoosition with score>=50
    uint64_t total_nodes=0;
    uint64_t last_nodes=0;
        
    Time::point start = Time::now();
    Time::point last_time=0;
    Time::point dead_line = start + dur;
    
    while(dead_line > Time::now())
    {
        //std::cout << num_compos << std::endl;
        pos.clear();
        //std::cout << (double)rand() / (double)RAND_MAX << std::endl;
        
        total_nodes++;
        for (PieceType pt=KING; pt>=PAWN; --pt)
            for(Color c=WHITE; c<=BLACK; ++c)
            {
                Piece p = make_piece(c,pt);
                
                int n=0;
                if(t) {
                    double rn = rand()/(double)RAND_MAX * am_limits[p];
                    n = find_value(cp_amount[p],rn);
                }
                else {
                    // 0-8 pawns, 1 king, Queen and 0-2 other pieces 
                    n = pt==PAWN? rand()%9 : pt==KING || pt==QUEEN? 1 : rand()%3; 
                }
                
                // squares where p can be placed (by statistics and empty sqrs)
                Bitboard possibleBB = t? stat_sqrs[p] & ~pos.pieces() :
                                         ~pos.pieces();
                
                //std::cout << pos.fen() << ":\n" << Bitboards::pretty(pos.pieces())<<std::endl;
                
                //std::cout << Bitboards::pretty(possibleBB) << std::endl;
                while(n && possibleBB)
                {
                    Square sq=SQ_NONE;
                    if(t) {
                        double rsq = rand()/(double)RAND_MAX * sq_limits[p];
                        sq = (Square)find_value(cp_squares[p],rsq);
                    }
                    else {
                        // 8-55 for pawns and 0-63 for other pieces
                        sq = pt==PAWN? (Square)(rand()%48 + 8) : (Square)(rand()%64); 
                    }
                    
                    //if (pos.piece_on(sq) == NO_PIECE)
                    if (possibleBB & sq)
                    {
                        pos.put_piece(sq,c,pt);
                        possibleBB ^= sq;
                        n--;
                        //std::cout << p << " placed" << std::endl;
                    }
                }
            }
        //std::cout << pos.fen() << std::endl;
        
        pos.set_checkers(pos.attackers_to(pos.king_square(pos.side_to_move()))
            & pos.pieces(~pos.side_to_move()));
        
        //std::cout << pos.fen() << std::endl;
        
        if (!pos.is_legal())
        {
            //std::cout << "NOT_LEGAL" << std::endl;
            gen_stats[GC_NOT_LEGAL]++;
            continue;
        }
        
        Search::Composition compos = Search::Composition(pos);

        if (!compos.legal_mate_in_2())
        {
            //std::cout << "NOT_MATE_IN_2" << std::endl;
            gen_stats[GC_NOT_MATE_IN_2]++;
            continue;
        }

       
        //std::cout << "mate in 2" << std::endl;
        
        compos.calc_score();
        
        themes_nb[compos.themes]++;
        bonuses_nb[compos.bonuses]++;
        penalties_nb[compos.penalties]++;

        
        if (compos.score < 0)
        {
            //std::cout << "SCORE_NEG" << std::endl;
            gen_stats[GC_SCORE_NEG]++; 
        }
        else if (compos.score < 50)
        {
            //std::cout << "SCORE_0_50" << std::endl;
            gen_stats[GC_SCORE_0_50]++; 
        }
        else
        {
            //std::cout << "SCORE_50_100" << std::endl;
            Time::point total_time = Time::now() - start;

            generated.push_back(DataAccess::GenCompos(pos.fen(), compos.themes,
                    compos.bonuses, compos.penalties, compos.score,
                    compos.theme_vars, total_nodes,total_nodes-last_nodes,
                    total_time, total_time-last_time, t));
            
            last_nodes  = total_nodes;
            last_time = total_time;
            
            //DataAccess::addCmp(pos.fen(),compos.themes, compos.bonuses, compos.penalties,
            //        compos.score,compos.theme_vars,total_nodes,total_nodes-last_nodes,
            //        total_time, total_time-last_time);
            
            if (compos.score < 100) gen_stats[GC_SCORE_50_100]++;
            else if (compos.score < 150) gen_stats[GC_SCORE_100_150]++;
            else gen_stats[GC_SCORE_150_PLUS]++;
        }
    }
    
    gen_stats[GC_TOTAL_TIME]  = dur;
    gen_stats[GC_TOTAL_NODES] = total_nodes;
    
    time_t insert_start, insert_end;
    char insert_tm[80];
     
    time (&insert_start);
    Time::point insert_msec = Time::now();
    
    for (std::vector<DataAccess::GenCompos>::iterator it=generated.begin();
            it!=generated.end(); ++it)
    {
        DataAccess::addCmp(*it);
    }
    
    insert_msec = Time::now() - insert_msec;
    time (&insert_end);
        
    std::cout << "  Inserted  " << generated.size()
            << " generated compositions with score >= 50 to DB in ";
    if (insert_msec<1000){
        std::cout << "00:00:00:" << insert_msec << std::endl; 
    }
    else {
        insert_end -= insert_start;
        strftime (insert_tm,80,"%X",gmtime(&insert_end));
        std::cout << insert_tm << std::endl;
    }
    
    /*if (db_timer<1000)
        std::cout << db_timer << " msec." << std::endl;
    else if (db_timer<(60*1000))
        std::cout << (double)db_timer/1000 << " sec." << std::endl;
    else if (db_timer<(60*60*1000))
        std::cout << (double)db_timer/60000 << " min." << std::endl;    
    else 
        std::cout << (double)db_timer/3600000 << " hrs." << std::endl;
    */
    
    
    DataAccess::updateGS(gen_stats,themes_nb,bonuses_nb,penalties_nb);
    
    /*
    if (db_timer<1000)
        std::cout << db_timer << " msec." << std::endl;
    else if (db_timer<(60*1000))
        std::cout << (double)db_timer/1000 << " sec." << std::endl;
    else if (db_timer<(60*60*1000))
        std::cout << (double)db_timer/60000 << " min." << std::endl;    
    else 
        std::cout << (double)db_timer/3600000 << " hrs." << std::endl;
    */
    if(debug) DataAccess::getGS(print_callback);
    
    DataAccess::close_db(CREATED);
}

//find out value in region where d exists
int Engine::find_value(std::multimap<double,int> &m, double d)
{
    for(std::map<double,int>::reverse_iterator rit=m.rbegin(); rit!=m.rend(); ++rit)
    {
        if (d < rit->first)
        {
            //std::cout << rit->first << std::endl;
            continue;
        }
        else if (d && d==rit->first) // for all except 0
            return rit->second;
            
        rit--;
        return rit->second;
    }

    return m.begin()->second;
}

void adm_improve_loop(Position& pos, Position& best_pos, double& best_score,
        Transforms& best_trans, Themes& best_t, Bonuses& best_b, Penalties& best_p,
        int* theme_vars, double init_score, Transforms trans, uint64_t* nodes,
        uint64_t* not_legal, uint64_t* not_mate_in_2, uint64_t* score_less, 
        uint64_t* improved, Color next_c, PieceType next_pt, int d)
{        
    //std::cout << "LEVEL " << d << " -------------------->" << std::endl;
    Bitboard oldChecks=pos.checkers();

    Bitboard piecesBB = pos.pieces();
    //std::cout << "Pieces on squares: \n" << Bitboards::pretty(piecesBB) << std::endl;
    
    Bitboard emptyBB = ~piecesBB;
    //std::cout << "Empty squares: \n" << Bitboards::pretty(emptyBB) << std::endl;
    
    
    Bitboard checkCandidatesBB = attacks_bb<ROOK>(pos.king_square(pos.side_to_move()),piecesBB)
            | attacks_bb<BISHOP>(pos.king_square(pos.side_to_move()),piecesBB)
            | pos.attacks_from<KNIGHT>(pos.king_square(pos.side_to_move()));
               
    //std::cout << "Check candidates: \n" << Bitboards::pretty(checkCandidatesBB) << std::endl;
    
    //std::cout << "Checkers: \n" << Bitboards::pretty(pos.checkers()) << std::endl;
    
    //ADD
    while (emptyBB)
    {
        Square s = pop_lsb(&emptyBB);
        //std::cout << "depth " << d << ": square " << s << std::endl;
        for (Color c=next_c; c<=BLACK; ++c)
        {            
            for (PieceType pt=next_pt; pt >= PAWN; --pt)
            {
                //nodes[d-1]++;
                if (pt==PAWN && (NonLegalPawnsBB & s) )
                    continue;
                  
                //std::cout << "on " << s << " placing " << PieceStr[make_piece(c,pt)] << "...";

                pos.put_piece(s,c,pt);
                
                //std::cout << "ok" << std::endl;
                
                (*nodes)++;
                
                //std::cout << "on " << s << " placed " << PieceStr[make_piece(c,pt)] << std::endl;
                //std::cout << pos.fen() << std::endl;
                
                //update checkers if needed
                if(checkCandidatesBB & s)
                {
                    oldChecks = pos.checkers();
                    pos.set_checkers(pos.attackers_to(pos.king_square(pos.side_to_move()))
                        & pos.pieces(~pos.side_to_move()));
                    
                    //if (oldChecks != pos.checkers())
                    //    std::cout << "level " << d << ": on " << s << " placed " << make_piece(c,pt) << std::endl;
                }
                
                //if (d==1 && s>=Square(12) && pt==KNIGHT)
                //{
                //    std::cout << "checkers after placing:\n" << Bitboards::pretty(pos.checkers()) << std::endl;
                //}

                if (d!=1)
                {             
                    if(pt!=PAWN)
                        adm_improve_loop(pos, best_pos, best_score, best_trans, best_t,
                                best_b, best_p, theme_vars, init_score,
                                create_transform(trans,d-1,ADD),nodes, not_legal, 
                                not_mate_in_2, score_less, improved, c, pt-1, d-1);
                    else if(c==WHITE)
                        adm_improve_loop(pos, best_pos, best_score, best_trans, best_t,
                                best_b, best_p, theme_vars, init_score,
                                create_transform(trans,d-1,ADD), nodes, not_legal, 
                                not_mate_in_2, score_less,  improved, c+1, QUEEN, d-1);

                    //std::cout << d << ": " << pos.fen() << std::endl;
                    //std::cout << "on " << s << " placed " << PieceStr[make_piece(c,pt)] << std::endl;

                    pos.remove_piece(s,c,pt);
                    
                     //update checkers if needed
                    if (pos.checkers() != oldChecks)
                    {
                         //std::cout << "changed:\n" << Bitboards::pretty(pos.checkers()) << std::endl;
                         //std::cout << "old:\n" << Bitboards::pretty(oldChecks) << std::endl;
                        pos.set_checkers(oldChecks);
                    }
                    //std::cout << "done" << std::endl;
                    continue;
                }
                
                //std::cout << "legal?..." << std::endl;
                if (!pos.is_legal())
                {
                    (*not_legal)++;
                    
                    pos.remove_piece(s,c,pt);
                    
                    //update checkers if needed
                    if (pos.checkers() != oldChecks)
                        pos.set_checkers(oldChecks);                    
                    //std::cout << "not legal" << std::endl;
                    continue;
                }
                
                //std::cout << "mate in 2?..." << std::endl;
                Search::Composition compos = Search::Composition(pos,3);
                //std::cout << "compos done." << std::endl;
                if(!compos.legal_mate_in_2())
                {
                    (*not_mate_in_2)++;
                    
                    pos.remove_piece(s,c,pt);
                    
                    //update checkers if needed
                    if (pos.checkers() != oldChecks)
                        pos.set_checkers(oldChecks);
                        
                    //std::cout << "not mate in 2" << std::endl;
                    continue;
                }

                //std::cout << "better score?..." << std::endl;
                compos.calc_score();
                
                // count improvments
                (*score_less) += compos.score<=init_score? 1 : 0;
                (*improved) += compos.score>init_score? 1 : 0;
                
                // find best improvment
                if (compos.score > best_score)
                {
                    best_pos = pos;
                    best_score = compos.score;
                    best_trans = create_transform(trans,d-1,ADD);
                    best_t = compos.themes;
                    best_b = compos.bonuses;
                    best_p = compos.penalties;
                    std::memcpy(theme_vars, compos.theme_vars, sizeof(int)*(DATA_NB+1));
                    //std::memcpy(theme_vars, compos.theme_vars, sizeof(compos.theme_vars));
                    
                    pos.remove_piece(s,c,pt);
                        
                    //update checkers if needed
                    if (pos.checkers() != oldChecks)
                        pos.set_checkers(oldChecks);
                }
                else
                {
                    pos.remove_piece(s,c,pt);
                        
                    //update checkers if needed
                    if (pos.checkers() != oldChecks)
                        pos.set_checkers(oldChecks);
                        
                    //std::cout << "worse score." << std::endl;
                    continue;
                }
                
            }
        }
    }
    
    //DEL
    piecesBB ^= pos.pieces(KING);
    
    while(piecesBB)
    {
        Square s = pop_lsb(&piecesBB);
        Piece p = pos.piece_on(s);
        pos.remove_piece(s, color_of(p),type_of(p));

        (*nodes)++;

        //std::cout << "from " << s << " deleted " << PieceStr[p] << std::endl;
        //std::cout << pos.fen() << std::endl;
            
        //update checkers if needed
        if(checkCandidatesBB & s)
        {
            oldChecks = pos.checkers();
            pos.set_checkers(pos.attackers_to(pos.king_square(pos.side_to_move()))
                & pos.pieces(~pos.side_to_move()));
        }
               
        if (d!=1)
        {
            adm_improve_loop(pos, best_pos, best_score, best_trans, best_t, best_b,
                    best_p, theme_vars, init_score, create_transform(trans,d-1,DEL),
                    nodes, not_legal, not_mate_in_2, score_less,  improved,
                    WHITE, QUEEN, d-1);
            
            pos.put_piece(s,color_of(p), type_of(p));
                   
            //update checkers if needed
            if(checkCandidatesBB & s)
                pos.set_checkers(oldChecks);
            
            continue;
        }

        if (!pos.is_legal())
        {
            (*not_legal)++;
            
            pos.put_piece(s, color_of(p), type_of(p));

            //update checkers if needed
            if(checkCandidatesBB & s)
                pos.set_checkers(oldChecks);
            //std::cout << "not legal" << std::endl;
            continue;
        }

        Search::Composition compos = Search::Composition(pos,3);
        if(!compos.legal_mate_in_2())
        {
            (*not_mate_in_2)++;
            
            pos.put_piece(s, color_of(p), type_of(p));
                    
            //update checkers if needed
            if(checkCandidatesBB & s)
                pos.set_checkers(oldChecks);
                       
            //std::cout << "not mate in 2" << std::endl;
            continue;
        }

        compos.calc_score();
        
        // count improvments
        (*score_less) += compos.score<=init_score? 1 : 0;
        (*improved) += compos.score>init_score? 1 : 0;
                
        // find best improvment
        if (compos.score > best_score)
        {
            best_pos = pos;
            best_score = compos.score;
            best_trans = create_transform(trans,d-1,DEL);
            best_t = compos.themes;
            best_b = compos.bonuses;
            best_p = compos.penalties;
            std::memcpy(theme_vars, compos.theme_vars, sizeof(int)*(DATA_NB+1));
            //std::memcpy(theme_vars, compos.theme_vars, sizeof(compos.theme_vars));
            
            pos.put_piece(s, color_of(p), type_of(p));
                   
            //update checkers if needed
            if(checkCandidatesBB & s)
                pos.set_checkers(oldChecks);
                   
            //DataAccess::addCmp(pos.fen(),ts,new_score);
        }
        else
        {
            //std::cout << "worse score." << std::endl;

            pos.put_piece(s,color_of(p), type_of(p));

            //update checkers if needed
            if(checkCandidatesBB & s)
                pos.set_checkers(oldChecks);
        }
    }

    //MOV
    int moves;
    
    for (int i=0; i<4; ++i)
    {
        moves=0;
        //std::cout << d << ": SHIFT: " << move_deltas[i] << std::endl;
        while (pos.move_board( move_deltas[i]))
        {
            //std::cout << pos.fen() << std::endl;
            
            moves++;
            (*nodes)++;
            
            if (!pos.is_legal())
            {
                (*not_legal)++;
                
                //std::cout << "not legal" << std::endl;
                break;
            }

            if (d!=1)
            {
                adm_improve_loop(pos, best_pos, best_score, best_trans, best_t, best_b,
                        best_p, theme_vars, init_score, create_transform(trans,d-1,MOV),
                        nodes, not_legal, not_mate_in_2, score_less, improved,
                        next_c, next_pt, d-1);
                continue;
            }

            Search::Composition compos = Search::Composition(pos,3);

            if(!compos.legal_mate_in_2())
            {
                (*not_mate_in_2)++;
                continue;
            }

            compos.calc_score();
            
            // count improvments
            (*score_less) += compos.score<=init_score? 1 : 0;
            (*improved) += compos.score>init_score? 1 : 0;

            // find best improvment
            if (compos.score > best_score)
            {
                best_pos = pos;
                best_score = compos.score;
                best_trans = create_transform(trans,d-1,MOV);
                best_t = compos.themes;
                best_b = compos.bonuses;
                best_p = compos.penalties;
                std::memcpy(theme_vars, compos.theme_vars, sizeof(int)*(DATA_NB+1));
                //std::memcpy(theme_vars, compos.theme_vars, sizeof(compos.theme_vars));
            }
        }

        //std::cout << pos.fen() << std::endl;
        //std::cout << Bitboards::pretty(pos.checkers()) << std::endl;
        //return to initial position
        if(moves)
        {
            //std::cout << "moving back " << -move_deltas[i] * moves << "...";
            pos.move_board( -move_deltas[i] * moves);
            //std::cout << "ok" << std::endl;
        }
        //std::cout << i << std::endl;
    }
}

bool Engine::adm_improve(Position& pos, int ID, short lvl,
        std::vector<DataAccess::AdmCompos>* imp_compos)
{
    Search::Composition compos = Search::Composition(pos);
    compos.calc_score();
    double init_score = compos.score;
    double best_score = init_score;
    Position best_pos;
    Transforms best_trans=0;
    Themes best_t=0;
    Bonuses best_b=0;
    Penalties best_p=0;
    int theme_vars[DATA_NB+1];
    
    uint64_t nodes[4], not_legal[4], not_mate_in_2[4], score_less[4], improved[4];
    Time::point times[4];
    time_t now, imp_start, imp_end;
    char now_buf[80];
    
    time(&now);
    strftime (now_buf,80,"%X [%d/%m/%y]",localtime(&now));
    std::cout << "\nStart improving #"<< ID <<" at " << now_buf << std::endl;
    
    short limit = lvl>=3? 3 : lvl;
    
    for (int i=0; i<limit; ++i)
    {
        nodes[i]=0;
        not_legal[i]=0;
        not_mate_in_2[i]=0; 
        score_less[i]=0;
        improved[i]=0;
        char imp_tm[80];
        
        time(&imp_start);
        times[i] = Time::now();

        adm_improve_loop(pos, best_pos, best_score, best_trans, best_t, best_b,
                best_p, theme_vars, init_score, 0, &(nodes[i]), &(not_legal[i]),
                &(not_mate_in_2[i]), &(score_less[i]), &(improved[i]), WHITE, QUEEN, i+1);

        times[i] = Time::now() - times[i];
        time(&imp_end);
                  
        imp_end -= imp_start;
        strftime (imp_tm,80,"%T",gmtime(&imp_end));

        std::cout << "  LVL"<<i+1<<": nodes(tot/n_leg/n_mi2/s_les/imp): " 
            <<nodes[i]<<"/"<<not_legal[i]<<"/"<<not_mate_in_2[i]
            <<"/"<<score_less[i] <<"/"<<improved[i] << ", time: ";
        // if < 1 sec print msecs
        if(times[i]<1000)
            std::cout << "00:00:00:"<<times[i] << std::endl;
        else std::cout << imp_tm << std::endl;    }

    nodes[3]=0;
    not_legal[3]=0;
    not_mate_in_2[3]=0; 
    score_less[3]=0;
    improved[3]=0;
    
    // if not improved go to depth 4
    if (lvl>3 && best_score == init_score)
    {
        std::cout << "\n  Not improved yet. Going to level 4..." << std::endl; 
        
        char imp_tm[80];
        
        time(&imp_start);
        times[3] = Time::now();

        adm_improve_loop(pos, best_pos, best_score, best_trans, best_t, best_b, 
                best_p, theme_vars, init_score, 0, &(nodes[3]), &(not_legal[3]),
                &(not_mate_in_2[3]), &(score_less[3]), &(improved[3]), WHITE, QUEEN, 4);

        times[3] = Time::now() - times[3];
        time(&imp_end);
                  
        imp_end -= imp_start;
        
        std::cout << "  LVL4: nodes(tot/n_leg/n_mi2/s_les/imp): " 
            <<nodes[3]<<"/"<<not_legal[3]<<"/"<<not_mate_in_2[3]
            <<"/"<<score_less[3] <<"/"<<improved[3] << ", time: ";
        // if < 1 sec print msecs
        if (times[3] >= (24*60*60*1000))
        {
            strftime (imp_tm,80,"%dd. %T",gmtime(&imp_end));
            std::cout << imp_tm << std::endl;
        }
        else
        {
            strftime (imp_tm,80,"%T",gmtime(&imp_end));
            std::cout << imp_tm << std::endl;
        }
    }
    else times[3]=0;
    
    if (best_score > init_score)
    {
        if(imp_compos)
            imp_compos->push_back(DataAccess::AdmCompos(ID,best_pos.fen(),best_t,best_b,
                    best_p,best_score,theme_vars,best_trans,nodes, not_legal,
                    not_mate_in_2, score_less, improved, times));
        
         std::cout << "\n  Best imp: "<<best_pos.fen()
                 <<"\n  Transforms: [ ";
         for (int i=0; i<4; ++i)
         {
             TransformType tt = type_of(best_trans, i); 
             if(tt)
                 std::cout <<  DataAccess::TransformTypeStr[tt] << " ";
             else break;
         }
         std::cout << "], score (old/new/imp): " <<init_score <<"/"<<best_score
                 <<"/"<<best_score-init_score<< std::endl;

         return true;
    }
                
    return false;
}

// perform best DEL at each iteration - greedy algorithm
void del_improve_loop(Position& pos, double& best_score, Themes& best_t,
        Bonuses& best_b, Penalties& best_p, int* theme_vars, int& dels)
{        
    Bitboard oldChecks=pos.checkers();
    Bitboard piecesBB = pos.pieces(PAWN,KNIGHT) | pos.pieces(BISHOP,ROOK) | pos.pieces(QUEEN);
    
    Bitboard checkCandidatesBB = attacks_bb<ROOK>(pos.king_square(pos.side_to_move()),piecesBB)
                | attacks_bb<BISHOP>(pos.king_square(pos.side_to_move()),piecesBB)
                | pos.attacks_from<KNIGHT>(pos.king_square(pos.side_to_move()));
    
    Square best_del = SQ_NONE;

    while(piecesBB)
    {
        Square s = pop_lsb(&piecesBB);
        Piece p = pos.piece_on(s);
        pos.remove_piece(s, color_of(p),type_of(p));

        //update checkers if needed
        if(checkCandidatesBB & s)
        {
            oldChecks = pos.checkers();
            pos.set_checkers(pos.attackers_to(pos.king_square(pos.side_to_move()))
                & pos.pieces(~pos.side_to_move()));
        }

        if (!pos.is_legal())
        {
            pos.put_piece(s, color_of(p), type_of(p));
            
            //update checkers if needed
            if(checkCandidatesBB & s)
                pos.set_checkers(oldChecks);
            
            continue;
        }
        
        Search::Composition compos = Search::Composition(pos,3);
        if(!compos.legal_mate_in_2())
        {
            pos.put_piece(s, color_of(p), type_of(p));
                    
            //update checkers if needed
            if(checkCandidatesBB & s)
                pos.set_checkers(oldChecks);
                       
            continue;
        }

        compos.calc_score();
        
        if (compos.score > best_score)
        {
            best_score = compos.score;
            best_t = compos.themes;
            best_b = compos.bonuses;
            best_p = compos.penalties;
            best_del = s;
            std::memcpy(theme_vars, compos.theme_vars, sizeof(int)*(DATA_NB+1));
        }

        pos.put_piece(s,color_of(p), type_of(p));

        //update checkers if needed
        if(checkCandidatesBB & s)    
            pos.set_checkers(oldChecks);
    }
    
    if (best_del != SQ_NONE)
    {
        dels++;
        
        Piece p = pos.piece_on(best_del);
        pos.remove_piece(best_del, color_of(p),type_of(p));
            
        //update checkers if needed
        if(checkCandidatesBB & best_del)
        {
            pos.set_checkers(pos.attackers_to(pos.king_square(pos.side_to_move()))
                & pos.pieces(~pos.side_to_move()));
        }
        
        del_improve_loop(pos,best_score,best_t,best_b,best_p,theme_vars,dels);
    }
}

bool Engine::del_improve(Position& pos, int ID,std::vector<DataAccess::DelCompos>* imp_compos)
{
    assert(pos.is_legal());
    
    Search::Composition compos = Search::Composition(pos);
    
    assert(compos.legal_mate_in_2());
    
    compos.calc_score();
    
    double init_score = compos.score;
    double best_score = init_score;
    int dels=0;
    Themes best_t=0;
    Bonuses best_b=0;
    Penalties best_p=0;
    int theme_vars[4];
    
    Time::point timer = Time::now();
    
    del_improve_loop(pos,best_score,best_t,best_b,best_p,theme_vars,dels);
    
    timer = Time::now() - timer;
    
    
    // insert anyway, even if not improved, just transform 0 indicate that not imp
    if(imp_compos)
            imp_compos->push_back(DataAccess::DelCompos(ID, pos.fen(), best_t,
                    best_b, best_p, best_score, compos.theme_vars, dels, timer));
   
    if (best_score - init_score <=  0)
        return false;
    
    std::cout << "#"<<ID<<": " << dels <<" deletes, score (old/new/imp): "
            <<init_score <<"/"<<best_score<<"/"<<best_score-init_score<< std::endl;
    
    return true;
}

void Engine::improve_all(ComposType type, double score, short lvl)
{   
    time_t start, end;
    char imp_tm [80];

    time (&start);
    
    if(DataAccess::is_opened(CREATED))
        DataAccess::open_db(CREATED);

    switch(type)
    {
        case DEL_IMP:{
            std::vector<DataAccess::DelCompos> results;
            DataAccess::getCmpByScore(CREATED,Engine::del_imp_callback, score, (void*)&results);
            
            for (std::vector<DataAccess::DelCompos>::iterator it=results.begin();
                    it!=results.end(); ++it)
            {
                DataAccess::addCmp(*it);
            }
            std::cout << "Processed " << results.size() 
                    << " compositions and inserted into DB in ";
        } break;
        
        case ADM_IMP:{
            std::vector<DataAccess::AdmCompos> results;
            AdmPair admp(lvl,&results);
            DataAccess::getCmpByScore(CREATED,Engine::adm_imp_callback, score, (void*)(&admp));
            
            for (std::vector<DataAccess::AdmCompos>::iterator it=results.begin();
                    it!=results.end(); ++it)
            {
                DataAccess::addCmp(*it,false);
            }
            
            std::cout << "Improved " << results.size() 
                    << " compositions and inserted into DB in ";
        } break;
        
        case FULL_IMP:{
           std::vector<DataAccess::AdmCompos> results;
            AdmPair fullp(lvl,&results);
            DataAccess::getCmpByScore(DEL_IMP,Engine::adm_imp_callback, score, (void*)(&fullp));
            
            for (std::vector<DataAccess::AdmCompos>::iterator it=results.begin();
                    it!=results.end(); ++it)
            {
                DataAccess::addCmp(*it,true);
            }
            
            std::cout << "Improved " << results.size() 
                    << " compositions and inserted into DB in ";            
        } break;
        default: {
            std::cout << "Wrong composition type" << std::endl;
        }
    }
    
    DataAccess::close_db(CREATED);
              
    time(&end);
    end -= start;
    strftime (imp_tm,80,"%T",gmtime(&end));
    
    std::cout << imp_tm << std::endl;
}
