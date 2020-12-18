# -*- coding: utf-8 -*-
"""
Train the model
"""

import os
import sys
import utils
import pickle
import argparse
sys.path.append('../')
import models
import numpy as np
import environment


# 产生KNOB的函数
def generate_knob(action, method):
    if method == 'ddpg':
        # 根据DDPG，更新KNOB的函数
        return environment.gen_continuous(action)
    else:
        raise NotImplementedError('Not Implemented')


if __name__ == '__main__':

    # 读取命令行参数的参数解析器
    parser = argparse.ArgumentParser()
    parser.add_argument('--tencent', action='store_true', help='Use Tencent Server')
    parser.add_argument('--params', type=str, default='', help='Load existing parameters')
    parser.add_argument('--workload', type=str, default='write', help='Workload type [`read`, `write`, `readwrite`]')
    parser.add_argument('--instance', type=str, default='mysql1', help='Choose MySQL Instance')
    parser.add_argument('--method', type=str, default='ddpg', help='Choose Algorithm to solve [`ddpg`,`dqn`]')
    parser.add_argument('--memory', type=str, default='', help='add replay memory')
    parser.add_argument('--noisy', action='store_true', help='use noisy linear layer')
    parser.add_argument('--other_knob', type=int, default=0, help='Number of other knobs')
    parser.add_argument('--batch_size', type=int, default=3, help='Training Batch Size')
    parser.add_argument('--epoches', type=int, default=5000000, help='Training Epoches')
    parser.add_argument('--benchmark', type=str, default='ycsb', help='[sysbench, tpcc, ycsb]')
    parser.add_argument('--metric_num', type=int, default=63, help='metric nums')
    parser.add_argument('--default_knobs', type=int, default=5, help='default knobs')
    opt = parser.parse_args()

    # Create Environment
    env = environment.Server(wk_type=opt.workload, instance_name=opt.instance)

    # Build models
    ddpg_opt = dict()
    # network 赋值到 target_network时，所乘的系数。系数越小，更新速度越慢。
    ddpg_opt['tau'] = 0.01
    ddpg_opt['alr'] = 0.00001
    ddpg_opt['clr'] = 0.00001
    ddpg_opt['model'] = opt.params
    n_states = opt.metric_num
    # critic网络更新时，损失函数乘以的系数。
    gamma = 0.9
    memory_size = 100000
    num_actions = opt.default_knobs + opt.other_knob
    ddpg_opt['gamma'] = gamma
    ddpg_opt['batch_size'] = opt.batch_size
    ddpg_opt['memory_size'] = memory_size

    model = models.DDPG(
        n_states=n_states,
        n_actions=num_actions,
        opt=ddpg_opt,
        mean_var_path='mean_var.pkl',
        # not opt.noisy 是true
        ouprocess=not opt.noisy
    )

    if not os.path.exists('log'):
        os.mkdir('log')

    if not os.path.exists('save_memory'):
        os.mkdir('save_memory')

    if not os.path.exists('save_knobs'):
        os.mkdir('save_knobs')

    if not os.path.exists('save_state_actions'):
        os.mkdir('save_state_actions')

    if not os.path.exists('model_params'):
        os.mkdir('model_params')

    # save_knobs 不用，其他的都用。
    expr_name = 'train_{}_{}'.format(opt.method, str(utils.get_timestamp()))

    logger = utils.Logger(
        name=opt.method,
        log_file='log/{}.log'.format(expr_name)
    )

    if opt.other_knob != 0:
        logger.warn('USE Other Knobs')

    current_knob = environment.get_init_knobs()

    # decay rate
    sigma_decay_rate = 0.9
    step_counter = 0
    train_step = 0
    accumulate_loss = [0, 0]

    fine_state_actions = []

    # 如果之前已经定义了opt.memory的存在，就加载。
    if len(opt.memory) > 0:
        model.replay_memory.load_memory(opt.memory)
        print("Load Memory: {}".format(len(model.replay_memory)))

    # time for every step
    step_times = []
    # time for training
    train_step_times = []
    # time for setup, restart, test
    env_step_times = []
    # restart time
    env_restart_times = []
    # choose_action_time
    action_step_times = []

    # default=5000000
    for episode in xrange(opt.epoches):
        current_state, initial_metrics = env.initialize()
        logger.info("\n[Env initialized][Metric tps: {} lat: {} qps: {}]".format(
            initial_metrics[0], initial_metrics[1], initial_metrics[2]))


        # model.reset(sigma)
        t = 0
        while True:
            step_time = utils.time_start()
            state = current_state
            if opt.noisy:
                model.sample_noise()
            action_step_time = utils.time_start()

            #根据当前的状态选择下一步的action(DDPG 的接口)
            action = model.choose_action(state)
            # print "gejiake action:",action

            #return delay
            action_step_time = utils.time_end(action_step_time)

            #根据DDPG中action来对Knob进行更新,action to knob(environment 接口)
            current_knob = generate_knob(action, 'ddpg')
            logger.info("[ddpg] Action: {}".format(action))

            env_step_time = utils.time_start()

            # 将新Knob值运用在mysql中，然后计算reward ,next_state,external_metrics等。
            reward, state_, done, score, metrics, restart_time = env.step(current_knob)
            env_step_time = utils.time_end(env_step_time)
            logger.info(
                "\n[{}][Episode: {}][Step: {}][Metric tps:{} lat:{} qps:{}]Reward: {} Score: {} Done: {}".format(
                    opt.method, episode, t, metrics[0], metrics[1], metrics[2], reward, score, done
                ))
            env_restart_times.append(restart_time)

            next_state = state_

            # 更新DDPG中随时更新的网络，并在replay_memory中保存重要参数。
            model.add_sample(state, action, reward, next_state, done)

            #reward大的话证明好，保存良好的actions。
            if reward > 10:
                fine_state_actions.append((state, action))

            current_state = next_state
            train_step_time = 0.0

            if len(model.replay_memory) > opt.batch_size:
                losses = []
                train_step_time = utils.time_start()
                for i in xrange(2):
                    # Update the Actor and Critic with a batch data
                    losses.append(model.update())
                    print 'gejiake loss:',losses
                    train_step += 1

                #训练了两遍，所以要除以2
                train_step_time = utils.time_end(train_step_time)/2.0

                accumulate_loss[0] += sum([x[0] for x in losses])
                accumulate_loss[1] += sum([x[1] for x in losses])
                # print 'gejiake accumulate_loss:', accumulate_loss
                logger.info('[{}][Episode: {}][Step: {}] Critic: {} Actor: {}'.format(
                    opt.method, episode, t, accumulate_loss[0] / train_step, accumulate_loss[1] / train_step
                ))

            # all_step time
            step_time = utils.time_end(step_time)
            step_times.append(step_time)
            # env_step_time
            env_step_times.append(env_step_time)
            # training step time
            train_step_times.append(train_step_time)
            # action step times
            action_step_times.append(action_step_time)

            logger.info("[{}][Episode: {}][Step: {}] step: {}s env step: {}s train step: {}s restart time: {}s "
                        "action time: {}s"
                        .format(opt.method, episode, t, step_time, env_step_time, train_step_time,restart_time,
                                action_step_time))

            logger.info("[{}][Episode: {}][Step: {}][Average] step: {}s env step: {}s train step: {}s "
                        "restart time: {}s action time: {}s"
                        .format(opt.method, episode, t, np.mean(step_time), np.mean(env_step_time),
                                np.mean(train_step_time), np.mean(restart_time), np.mean(action_step_times)))

            t = t + 1
            step_counter += 1

            # save replay memory
            if step_counter % 10 == 0:
                model.replay_memory.save('save_memory/{}.pkl'.format(expr_name))
                utils.save_state_actions(fine_state_actions, 'save_state_actions/{}.pkl'.format(expr_name))
                # sigma = origin_sigma*(sigma_decay_rate ** (step_counter/10))

            # save network
            if step_counter % 5 == 0:
                model.save_model('model_params', title='{}_{}'.format(expr_name, step_counter))

            if done or score < -50:
                break







